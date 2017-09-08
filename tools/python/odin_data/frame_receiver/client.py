from odin_data.ipc_channel import IpcChannel, IpcChannelException
from odin_data.ipc_message import IpcMessage, IpcMessageException

import time
import datetime
import threading
import logging
import sys
import zmq
from struct import Struct

class FrameReceiverClient(object):
    
    def __init__(self):

        # create logger
        self.logger = logging.getLogger('frame_receiver_client')
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

        # Create the appropriate IPC channels
        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_REQ)
        self.ctrl_channel.connect('tcp://127.0.0.1:5000')
        
        self._run = True
                
    def run(self):
        
        self.logger.info("Frame receiver client starting up")
        
        msg = IpcMessage('cmd', 'configure')
        msg.set_param('test', {'list': True})
        self.ctrl_channel.send(msg.encode())
        
        pollevts = self.ctrl_channel.poll(1000)
        if pollevts == IpcChannel.POLLIN:
            reply = IpcMessage(from_str=self.ctrl_channel.recv())
            self.logger.info("Got response: {}".format(reply))
        
def main():        
        app = FrameReceiverClient()
        app.run()

if __name__ == "__main__":
    main()