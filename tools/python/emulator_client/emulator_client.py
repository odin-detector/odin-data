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
 
    # Define legal commands
    LegalCommands = {
                     "start" : "startd\n\r",
                     "stop"  : "stopd\n\r\r"
                     }
    
    def __init__(self, host, port, timeout):

        self.host     = host #'192.168.0.111'
        self.port     = port # 4321
        self.timeout  = timeout
        
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
            


    def execute(self, command):
        
        if not command in EmulatorClient.LegalCommands:
            raise EmulatorClientError("Illegal command %s specified" % command)
        
        # Transmit command
        try:
            bytesSent = self.sock.send(EmulatorClient.LegalCommands[command])
        except socket.error, e:
            if self.sock:
                self.sock.close()
            raise EmulatorClientError("Error sending %s command: %s" % (command, e))
            
        if bytesSent != len(EmulatorClient.LegalCommands[command]):
            print "Failed to transmit %s command properly" % command
        else:
            print "Transmitted %s command" % command
   

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="EmulatorClient - control hardware emulator start/stop")
    
    parser.add_argument('--host', type=str, default='192.168.0.111', 
                        help="select emulator IP address")
    parser.add_argument('--port', type=int, default=4321,
                        help='select emulator IP port')
    parser.add_argument('--timeout', type=int, default=5,
                        help='set TCP connection timeout')
    parser.add_argument('command', choices=EmulatorClient.LegalCommands.keys(), default="start",
                        help="specify which command to send to emulator")

    args = parser.parse_args()
    try:
        client = EmulatorClient(args.host, args.port, args.timeout)
        client.execute(args.command)
    except Exception as e:
        print e
        
