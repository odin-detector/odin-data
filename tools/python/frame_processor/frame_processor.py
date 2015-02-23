from frame_receiver.ipc_channel import IpcChannel, IpcChannelException
from frame_receiver.ipc_message import IpcMessage, IpcMessageException
from frame_processor_config import FrameProcessorConfig

import time
import threading

class FrameProcessor(object):
    
    def __init__(self):

        # Instantiate a configuration container object, which will be populated
        # with sensible default values
        self.config = FrameProcessorConfig("FrameProcessor - test harness to simulate operation of FrameProcessor application")
                
        # Create the appropriate IPC channels
        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_REQ)
        self.ready_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_SUB)
        self.release_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_PUB)
        
        # Create the thread to handle frame processing
        self.frame_processor = threading.Thread(target=self.process_frames)
    
                
    def run(self):

        # Connect the IPC channels
        self.ctrl_channel.connect(self.config.ctrl_endpoint)
        self.ready_channel.connect(self.config.ready_endpoint)
        self.release_channel.connect(self.config.release_endpoint)
        
        # Ready channel subscribes to all topics
        self.ready_channel.subscribe(b'')
        
        # Launch the frame processing thread
        self.frame_processor.start()
        
        for i in range(10):
             
            #msg = "HELLO " + str(i)
            msg = IpcMessage(msg_type='cmd', msg_val='status')
            print "Sending message", msg
            self.ctrl_channel.send(msg.encode())
             
            reply = self.ctrl_channel.recv()
            reply_decoded = IpcMessage(from_str=reply)
            print "Got reply, msg_type =", reply_decoded.get_msg_type(), "val =", reply_decoded.get_msg_val()
            time.sleep(0.01) 
        
    def process_frames(self):
        
        while 1:
            
            ready_msg = self.ready_channel.recv() 
            ready_decoded = IpcMessage(from_str=ready_msg)
            
            if ready_decoded.get_msg_type() == 'notify' and ready_decoded.get_msg_val() == 'frame_ready':
            
                frame_number = ready_decoded.get_param('frame')
                print "Got frame ready notification for frame", frame_number
                
                release_msg = IpcMessage(msg_type='notify', msg_val='frame_release')
                release_msg.set_param('frame', frame_number)
                self.release_channel.send(release_msg.encode())
                
            else:
                
                print "Got unexpected message on ready notification channel:", ready_decoded
        
if __name__ == "__main__":
        
        fp = FrameProcessor()
        fp.run()
        