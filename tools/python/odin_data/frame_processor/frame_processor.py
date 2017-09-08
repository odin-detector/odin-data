from odin_data.ipc_channel import IpcChannel, IpcChannelException
from odin_data.ipc_message import IpcMessage, IpcMessageException
from odin_data.shared_buffer_manager import SharedBufferManager, SharedBufferManagerException
from frame_processor_config import FrameProcessorConfig
from percival_emulator_frame_decoder import PercivalEmulatorFrameDecoder, PercivalFrameHeader, PercivalFrameData
from excalibur_frame_decoder import ExcaliburFrameDecoder, ExcaliburFrameHeader, ExcaliburFrameData

import time
import datetime
import threading
import logging
import sys
from struct import Struct

class FrameProcessor(object):
    
    def __init__(self):

        # Initialise the logging module with log messages directed to stdout
        #logging.basicConfig(format='%(asctime)s %(levelname)s FrameProcessor - %(message)s', level=logging.DEBUG)
        #ch = logging.StreamHandler(sys.stdout)
        #logging.addHandler(ch)

        # create logger
        self.logger = logging.getLogger('frame_processor')
        self.logger.setLevel(logging.DEBUG)
        
        # create console handler and set level to debug
        ch = logging.StreamHandler(sys.stdout)
        ch.setLevel(logging.DEBUG)
        
        # create formatter
        formatter = logging.Formatter('%(asctime)s %(levelname)s %(name)s - %(message)s')
        
        # add formatter to ch
        ch.setFormatter(formatter)
        
        # add ch to logger
        self.logger.addHandler(ch)

        # Instantiate a configuration container object, which will be populated
        # with sensible default values
        self.config = FrameProcessorConfig("FrameProcessor", "FrameProcessor - test harness to simulate operation of FrameProcessor application")
                
        # Create the appropriate IPC channels
        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_REQ)
        self.ready_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_SUB)
        self.release_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_PUB)

        # Zero frames recevied counter
        self.frames_received = 0
        
        # Create the thread to handle frame processing
        self.frame_processor = threading.Thread(target=self.process_frames)
        self.frame_processor.daemon = True
    
        self._run = True
        
    def map_shared_buffer(self):
        
        success = False
        
        # Check if the current configuration object has a shared buffer name defined, otherwise request one from the
        # upstream frameReceiver
        if self.config.sharedbuf is None:
            if not self.request_shared_buffer_config():
                return success
            
        # Map the shared buffer manager
        try:
            self.shared_buffer_manager = SharedBufferManager(self.config.sharedbuf, boost_mmap_mode=self.config.boost_mmap_mode)
            success = True
        except SharedBufferManagerException as e:
            self.logger.error("Failed to create shared buffer manager: %s" % str(e))
            
        return success
                
    def request_shared_buffer_config(self):
        
        success = False

        max_request_retries = 10
        max_reply_retries = 10
        
        request_retries = 0
        config_request = IpcMessage(msg_type='cmd', msg_val='request_buffer_config')        
        
        while success is False and request_retries < max_request_retries:

            self.logger.debug("Sending buffer config request {}".format(request_retries + 1))
            self.release_channel.send(config_request.encode())
            reply_retries = 0
            
            while success is False and reply_retries < max_reply_retries:
                if self.ready_channel.poll(100):
                    config_msg = self.ready_channel.recv()
                    config_decoded = IpcMessage(from_str = config_msg)
                    self.logger.debug(
                        'Got buffer configuration response with shared buffer name: {}'.format(
                            config_decoded.get_param('shared_buffer_name')
                        )
                    )
                    self.config.sharedbuf = config_decoded.get_param('shared_buffer_name')
                    success = True
                else:
                    reply_retries += 1
                    
            request_retries += 1
                    
        # temp hack
        if not success:
            self.logger.error("Failed to obtain shared buffer configuration")
            
        return success
    
    def run(self):
        
        self.logger.info("Frame processor starting up")

        # Connect the IPC channels
        self.ctrl_channel.connect(self.config.ctrl_endpoint)
        self.ready_channel.connect(self.config.ready_endpoint)
        self.release_channel.connect(self.config.release_endpoint)

        # Ready channel subscribes to all topics
        self.ready_channel.subscribe(b'')
                                  
        # Map the shared buffer manager - quit if this fails       
        if not self.map_shared_buffer():
            self._run = False
            return
        
        self.logger.info("Mapped shared buffer manager ID %d with %d buffers of size %d" % 
                     (self.shared_buffer_manager.get_manager_id(), 
                      self.shared_buffer_manager.get_num_buffers(),
                      self.shared_buffer_manager.get_buffer_size()))
                 
        if self.config.sensortype == 'percivalemulator':    
            self.frame_decoder = PercivalEmulatorFrameDecoder(self.shared_buffer_manager)
            self.logger.debug('Loaded frame decoder for PERCIVAL emulator sensor type')
        elif self.config.sensortype == 'excalibur':
            self.frame_decoder = ExcaliburFrameDecoder(self.shared_buffer_manager)
            self.logger.debug('Loaded frame decoder for EXCALIBUR sensor type')
        else:
            self.frame_decoder = None
            self.logger.error("Unrecognised sensor type specified: %s" % self.config.sensortype)
            return
        

        # Launch the frame processing thread
        self.frame_processor.start()
        
        try:
            while self._run:
                                
                if self.config.frames and self.frames_received >= self.config.frames:
                    self.logger.info("Specified number of frames (%d) received, terminating" % self.config.frames)
                    self._run = False
                else:   
                    msg = IpcMessage(msg_type='cmd', msg_val='status')
                    #self.logger.debug("Sending status command message")
                    self.ctrl_channel.send(msg.encode())
                    
                    reply = self.ctrl_channel.recv()
                    reply_decoded = IpcMessage(from_str=reply)
                        
                    #self.logger.debug("Got reply, msg_type = " + reply_decoded.get_msg_type() + " val = " + reply_decoded.get_msg_val())
                    time.sleep(1) 
        
        except KeyboardInterrupt:
            
            self.logger.info("Got interrupt, terminating")
            self._run = False
            
        self.frame_processor.join()
        self.logger.info("Frame processor shutting down")
        
    def process_frames(self):
        
        self.frame_header = Struct('<LLQQL')
        
        while self._run:
            
            if (self.ready_channel.poll(100)):

                ready_msg = self.ready_channel.recv() 
                ready_decoded = IpcMessage(from_str=ready_msg)
                
                if ready_decoded.get_msg_type() == 'notify' and ready_decoded.get_msg_val() == 'frame_ready':
                
                    frame_number = ready_decoded.get_param('frame')
                    buffer_id    = ready_decoded.get_param('buffer_id')
                    self.logger.debug("Got frame ready notification for frame %d buffer ID %d" %(frame_number, buffer_id))
                    
                    if not self.config.bypass_mode:
                        self.handle_frame(frame_number, buffer_id)
                    
                    release_msg = IpcMessage(msg_type='notify', msg_val='frame_release')
                    release_msg.set_param('frame', frame_number)
                    release_msg.set_param('buffer_id', buffer_id)
                    self.release_channel.send(release_msg.encode())
                    
                    self.frames_received += 1
                    
                elif ready_decoded.get_msg_type() == 'notify' and ready_decoded.get_msg_val() == 'buffer_config':
                    
                    shared_buffer_name = ready_decoded.get_param('shared_buffer_name')
                    self.logger.debug('Got shared buffer config notification with name %s' % (shared_buffer_name))
                    
                else:
                    
                    self.logger.error("Got unexpected message on ready notification channel: %s" % (ready_decoded))
        
        self.logger.info("Frame processing thread interrupted, terminating")
        
    def handle_frame(self, frame_number, buffer_id):
        
        self.frame_decoder.decode_header(buffer_id)
        self.logger.debug('Frame {:d} in buffer {:d} decoded header values:\n{:s}'.format(
            frame_number, buffer_id, self.frame_decoder.header_state_str()))
        
        if self.config.packet_state:
            self.logger.debug("Packet state : \n" + self.frame_decoder.packet_state_str())
        
        self.frame_decoder.decode_data(buffer_id)
        self.logger.debug("Frame start: " + ' '.join("0x{:04x}".format(val) for val in self.frame_decoder.data.pixels[:16]))
        self.logger.debug("Frame end  : " + ' '.join("0x{:04x}".format(val) for val in self.frame_decoder.data.pixels[-16:]))
        
def main():        
        fp = FrameProcessor()
        fp.run()

if __name__ == "__main__":
    main()