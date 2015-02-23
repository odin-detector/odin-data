from frame_receiver.ipc_channel import IpcChannel, IpcChannelException
from frame_receiver.ipc_message import IpcMessage, IpcMessageException
from frame_receiver.shared_buffer_manager import SharedBufferManager, SharedBufferManagerException
from frame_processor_config import FrameProcessorConfig

import time
import threading
import logging
from struct import Struct

class FrameProcessor(object):
    
    def __init__(self):

        logging.basicConfig(format='%(asctime)s %(levelname)s FrameProcessor - %(message)s', level=logging.DEBUG)
        
        # Instantiate a configuration container object, which will be populated
        # with sensible default values
        self.config = FrameProcessorConfig("FrameProcessor - test harness to simulate operation of FrameProcessor application")
                
        # Create the appropriate IPC channels
        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_REQ)
        self.ready_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_SUB)
        self.release_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_PUB)
        
        # Map the shared buffer manager
        self.shared_buffer_manager = SharedBufferManager(self.config.sharedbuf)
        
        # Create the thread to handle frame processing
        self.frame_processor = threading.Thread(target=self.process_frames)
        self.frame_processor.daemon = True
    
        self._run = True
                
    def run(self):
        
        logging.info("Frame processor starting up")
        
        logging.info("Mapped shared buffer manager ID %d with %d buffers of size %d" % 
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
                 
                msg = IpcMessage(msg_type='cmd', msg_val='status')
                logging.debug("Sending status command message")
                self.ctrl_channel.send(msg.encode())
                 
                reply = self.ctrl_channel.recv()
                reply_decoded = IpcMessage(from_str=reply)
                logging.debug("Got reply, msg_type = " + reply_decoded.get_msg_type() + " val = " + reply_decoded.get_msg_val())
                time.sleep(1) 
        
        except KeyboardInterrupt:
            
            logging.info("Got interrupt, terminating")
            self._run = False;
            self.frame_processor.join()
        
        logging.info("Frame processor shutting down")
        
    def process_frames(self):
        
        self.frame_header = Struct('QQQ')
        
        while self._run:
            
            if (self.ready_channel.poll(100)):

                ready_msg = self.ready_channel.recv() 
                ready_decoded = IpcMessage(from_str=ready_msg)
                
                if ready_decoded.get_msg_type() == 'notify' and ready_decoded.get_msg_val() == 'frame_ready':
                
                    frame_number = ready_decoded.get_param('frame')
                    buffer_id    = ready_decoded.get_param('buffer_id')
                    logging.debug("Got frame ready notification for frame %d buffer ID %d" %(frame_number, buffer_id))
                    
                    self.handle_frame(frame_number, buffer_id)
                    
                    release_msg = IpcMessage(msg_type='notify', msg_val='frame_release')
                    release_msg.set_param('frame', frame_number)
                    release_msg.set_param('buffer_id', buffer_id)
                    self.release_channel.send(release_msg.encode())
                    
                else:
                    
                    logging.error("Got unexpected message on ready notification channel:", ready_decoded)
        
        logging.info("Frame processing thread interrupted, terminating")
        
    def handle_frame(self, frame_number, buffer_id):
        
        header_raw = self.shared_buffer_manager.read_buffer(buffer_id, self.frame_header.size)
        header_vals = self.frame_header.unpack(header_raw)
        logging.debug("Frame number %d had header values: %s" % (frame_number, ' '.join([hex(val) for val in header_vals])))
        
        
if __name__ == "__main__":
        
        fp = FrameProcessor()
        fp.run()
        