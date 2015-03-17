'''
Created on March 10, 2015

@author: ckd27546
'''

import argparse, socket, time

class EmulatorClientError(Exception):
    
    def __init__(self, msg):
        self.msg = msg
        
    def __str__(self):
        return repr(self.msg)

class EmulatorClient(object):
 
    def __init__(self, host, port, timeout, duration):

        self.host     = host #'192.168.0.111'
        self.port     = port # 4321
        self.timeout  = timeout
        self.duration = duration
        
        # Define Start and Stop TCP commands
        self.command = "startd\n\rstopd\n\r\r"

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(self.timeout)
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
        
        startCmd = self.command[0:8]
        stopCmd  = self.command[8:]

        # Transmit Start command
        try:
            bytesSent = self.sock.send(startCmd)
        except socket.error, e:
            if self.sock:
                self.sock.close()
            raise EmulatorClientError("Error sending Start command: %s" % e)
            
        if bytesSent != 8:
            print "Failed to transmit start command properly"
        else:
            print "Transmitted Start command"

        print "Waiting %f seconds.." % self.duration        
        time.sleep( self.duration )
        
        # Transmit Stop command
        try:
            bytesSent = self.sock.send(stopCmd)
        except socket.error, e:
            if self.sock:
                self.sock.close()
            raise EmulatorClientError("Error sending Stop command: %s" % e)
        
        if bytesSent != 8:
            print "Failed to transmit stop command properly"
        else:
            print "Transmitted Stop command"

        # Close socket
        self.sock.close()
    

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="EmulatorClient - control hardware simulator start/stop")
    
    parser.add_argument('--host', type=str, default='192.168.0.111', 
                        help="select emulator IP address")
    parser.add_argument('--port', type=int, default=4321,
                        help='select emulator IP port')
    parser.add_argument('--timeout', type=int, default=5,
                        help='set TCP connection timeout')
    parser.add_argument('--duration', type=float, default=0.30,
                        help='duration between sending start and stop command')

    args = parser.parse_args()
    try:
        client = EmulatorClient(**vars(args))
        client.execute()
    except Exception as e:
        print e
        
