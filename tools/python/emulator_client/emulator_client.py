'''
Created on March 10, 2015

@author: ckd27546
'''

import argparse, socket, time
import numpy as np

class EmulatorClientError(Exception):
    
    def __init__(self, msg):
        self.msg = msg
        
    def __str__(self):
        return repr(self.msg)

class EmulatorClient(object):
 
    def __init__(self, host, port):

        self.host = host #'192.168.0.111'
        self.port = port # 4321
        
        # Define Start and Stop TCP commands
        self.command = np.empty(16, dtype=np.uint8)
        self.command[0] = 0x73  # s
        self.command[1] = 0x74  # t
        self.command[2] = 0x61  # a
        self.command[3] = 0x72  # r
        self.command[4] = 0x74  # t
        self.command[5] = 0x64  # d
        self.command[6] = 0x0A  # NL
        self.command[7] = 0x0D  # CR
        
        self.command[8]  = 0x73  # s
        self.command[9]  = 0x74  # t
        self.command[10] = 0x6F  # o
        self.command[11] = 0x70  # p
        self.command[12] = 0x64  # d
        self.command[13] = 0x0A  # NL
        self.command[14] = 0x0D  # CR
        self.command[15] = 0x0D  # CR

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(5)
        # Open TCP connection
        try:
            self.sock.connect((host, port))
        except socket.timeout:
            raise EmulatorClientError("Connecting to [%s:%d] timed out" % (host, port))
            
        except socket.error, e:
            if self.sock:
                self.sock.close()
            raise EmulatorClientError("Error connecting to [%s:%d]: '%s'" % (host, port, e))
            


    def execute(self):
        
        startCmd = self.command[0:8].tostring()
        
        # Transmit Start command
        bytesSent = self.sock.send(startCmd)
        if bytesSent != 8:
            print "Failed to transmit start command properly"
        else:
            print "Transmitted Start command"
        
        time.sleep(0.30)
        
        stopCmd = self.command[8:].tostring()
        if bytesSent != 8:
            print "Failed to transmit stop command properly"
        else:
            print "Transmitted Stop command"
        
        # Transmit Stop command
        bytesSent = self.sock.send(stopCmd)

        # Close socket
        self.sock.close()
    

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="EmulatorClient - control hardware simulator start/stop")
    
    parser.add_argument('--host', type=str, default='192.168.0.111', 
                        help="select destination host IP address")
    parser.add_argument('--port', type=int, default=4321,
                        help='select destination host IP port')

    
    args = parser.parse_args()
    try:
        client = EmulatorClient(**vars(args))
        client.execute()
    except Exception as e:
        print e
        
