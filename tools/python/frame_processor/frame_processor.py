from frame_receiver.ipc_channel import IpcChannel, IpcChannelException
from frame_receiver.ipc_message import IpcMessage, IpcMessageException
from frame_receiver.shared_buffer_manager import SharedBufferManager, SharedBufferManagerException
from frame_processor_config import FrameProcessorConfig
from percival_emulator_frame_decoder import PercivalEmulatorFrameDecoder, PercivalFrameHeader, PercivalFrameData

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
        self.logger = logging.getLogger('FrameProcessor')
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
        
        # Map the shared buffer manager
        self.shared_buffer_manager = SharedBufferManager(self.config.sharedbuf)
        
        self.frame_decoder = PercivalEmulatorFrameDecoder(self.shared_buffer_manager)
        
        # Zero frames recevied counter
        self.frames_received = 0
        
        # Create the thread to handle frame processing
        self.frame_processor = threading.Thread(target=self.process_frames)
        self.frame_processor.daemon = True
    
        self._run = True
                
    def run(self):
        
        self.logger.info("Frame processor starting up")
        
        self.logger.info("Mapped shared buffer manager ID %d with %d buffers of size %d" % 
                     (self.shared_buffer_manager.get_manager_id(), 
                      self.shared_buffer_manager.get_num_buffers(),
                      self.shared_buffer_manager.get_buffer_size()))

        # Connect the IPC channels
        self.ctrl_channel.connect(self.config.ctrl_endpoint)
        self.ready_channel.connect(self.config.ready_endpoint)
        self.release_channel.connect(self.config.release_endpoint)
        
        # Ready channel subscribes to all topics
        self.ready_channel.subscribe(b'')
        
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
            self._run = False;
            
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
                    
                else:
                    
                    self.logger.error("Got unexpected message on ready notification channel:", ready_decoded)
        
        self.logger.info("Frame processing thread interrupted, terminating")
        
    def handle_frame(self, frame_number, buffer_id):
        
        self.frame_decoder.decode_header(buffer_id)
        self.logger.debug("Frame %d in buffer %d decoded header values: frame_number %d state %d start_time %s packets_received %d" %
                      (frame_number, buffer_id, self.frame_decoder.header.frame_number, 
                       self.frame_decoder.header.frame_state, self.frame_decoder.header.frame_start_time.isoformat(),
                       self.frame_decoder.header.packets_received))

        self.frame_decoder.decode_data(buffer_id)
        self.logger.debug("Frame start: " + ' '.join("0x{:04x}".format(val) for val in self.frame_decoder.data.pixels[:32]))
        self.logger.debug("Frame end  : " + ' '.join("0x{:04x}".format(val) for val in self.frame_decoder.data.pixels[-32:]))
        
if __name__ == "__main__":
        
        fp = FrameProcessor()
        fp.run()
        