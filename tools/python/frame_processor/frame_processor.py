from frame_receiver.ipc_channel import IpcChannel, IpcChannelException
from frame_receiver.ipc_message import IpcMessage, IpcMessageException
from frame_processor_config import FrameProcessorConfig

import time

class FrameProcessor(object):
    
    def __init__(self):

        # Instantiate a configuration container object, which will be populated
        # with sensible default values
        self.config = FrameProcessorConfig("FrameProcessor - test harness to simulate operation of FrameProcessor application")
                
        # Create the appropriate IPC channels
        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_REQ)
    
                
    def run(self):

        self.ctrl_channel.connect(self.config.ctrl_endpoint)
        
        for i in range(10):
            
            #msg = "HELLO " + str(i)
            msg = IpcMessage(msg_type='cmd', msg_val='status')
            print "Sending message", msg
            self.ctrl_channel.send(msg.encode())
            
            reply = self.ctrl_channel.recv()
            reply_decoded = IpcMessage(from_str=reply)
            print "Got reply, msg_type =", reply_decoded.get_msg_type(), "val =", reply_decoded.get_msg_val()
            time.sleep(0.01) 
        

if __name__ == "__main__":
        
        fp = FrameProcessor()
        fp.run()
        