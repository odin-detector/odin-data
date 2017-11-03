from odin_data.ipc_channel import IpcChannel, IpcChannelException
from odin_data.ipc_message import IpcMessage, IpcMessageException

import argparse
import time
import datetime
import threading
import logging
import sys
import os
import json
import zmq
from struct import Struct
from json.decoder import JSONDecodeError

class FrameReceiverClient(object):
    
    def __init__(self):

        prog_name = os.path.basename(sys.argv[0])
        
        # Parse command line arguments
        self.args = self._parse_arguments(prog_name)
        
        # create logger
        self.logger = logging.getLogger(prog_name)
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
        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_DEALER)
        self.ctrl_channel.connect(self.args.ctrl_endpoint)
        
        self._run = True
    
    def _parse_arguments(self, prog_name=sys.argv[0]):
        
        parser = argparse.ArgumentParser(prog=prog_name, description='ODIN Frame Receiver Client')
        parser.add_argument('--ctrl', type=str, default='tcp://127.0.0.1:5000', dest='ctrl_endpoint',
                            help='Specify the IPC control channel endpoint URL')
        parser.add_argument('--config', type=argparse.FileType('r'), dest='config_file', nargs='?',
                            default=None, const=sys.stdin, 
                            help='Specify JSON configuration file to send as configure command')
        parser.add_argument('--shutdown', action='store_true',
                            help='Instruct the frame receiver to shut down')
        
        args = parser.parse_args()
        return args
    
    def run(self):
        
        self.logger.info("Frame receiver client starting up")
        
        self.logger.debug("Control IPC channel has identity {}".format(self.ctrl_channel.identity))
        
        if self.args.config_file is not None:
            self.do_config_cmd(self.args.config_file)

        if self.args.shutdown:
            self.do_shutdown_cmd()
            
    def do_config_cmd(self, config_file):

        try:        
            config_params = json.load(config_file)
            
            config_msg = IpcMessage('cmd', 'configure')
            for param, value in config_params.items():
                config_msg.set_param(param, value)
                
            self.logger.info("Sending configure command to frame receiver with specified parameters")
            self.ctrl_channel.send(config_msg.encode())
            self.await_response()
            
        except JSONDecodeError as e:
            self.logger.error("Failed to parse configuration file: {}".format(e))
            

    def do_shutdown_cmd(self):

        shutdown_msg = IpcMessage('cmd', 'shutdown')
        print(shutdown_msg)
        self.logger.info("Sending shutdown command to frame receiver")
        self.ctrl_channel.send(shutdown_msg.encode())
        self.await_response()

    def await_response(self, timeout_ms=1000):

        pollevts = self.ctrl_channel.poll(1000)
        if pollevts == IpcChannel.POLLIN:
            reply = IpcMessage(from_str=self.ctrl_channel.recv())
            self.logger.info("Got response: {}".format(reply))
                    
        
def main():        
        app = FrameReceiverClient()
        app.run()

if __name__ == "__main__":
    main()