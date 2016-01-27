'''
Created on March 10, 2015

@author: ckd27546

'''

import argparse, socket, time, sys

class EmulatorClientError(Exception):
    
    def __init__(self, msg):
        self.msg = msg
        
    def __str__(self):
        return repr(self.msg)


class EmulatorClient(object):
    ''' Test setting IP/MAC addresses in the firmware '''
 
    # Define legal commands #TODO: To include MAC/IP address?
    LegalCommands = {
                     "start"   : "startd\r\n",
                     "command" : "COMAND\n\r",
                     "stop"    : "\x20stopd\r\n"
                     }

    ## New stuff
    Eth_Dev_RW   = 0x00000001
    
    MAC_0_ADDR   = 0x80000006 
    MAC_1_ADDR   = 0x80000000 
    MAC_2_ADDR   = 0x80000012
    MAC_3_ADDR   = 0x8000000C 
    MAC_4_ADDR   = 0x8000001E                                       
    MAC_5_ADDR   = 0x80000018                                       
    MAC_6_ADDR   = 0x8000002A                                       
    MAC_7_ADDR   = 0x80000024 
    MAC_8_ADDR   = 0x80000036                                       
    MAC_9_ADDR   = 0x80000030                                       
    MAC_10_ADDR  = 0x80000042
    MAC_11_ADDR  = 0x8000003C
    MAC_12_ADDR  = 0x8000004E                                       
    MAC_13_ADDR  = 0x80000048                                       
    MAC_14_ADDR  = 0x8000005A                                       
    MAC_15_ADDR  = 0x80000054 

    IP_0_ADDR    = 0x60000000 
    IP_1_ADDR    = 0x70000000
    IP_2_ADDR    = 0x60000004
    IP_3_ADDR    = 0x70000004                                       
    IP_4_ADDR    = 0x60000008       
    IP_5_ADDR    = 0x70000008                                       
    IP_6_ADDR    = 0x6000000C                                       
    IP_7_ADDR    = 0x7000000C                                       
    IP_8_ADDR    = 0x60000010       
    IP_9_ADDR    = 0x70000010                                       
    IP_10_ADDR   = 0x60000014                                       
    IP_11_ADDR   = 0x70000014                                       
    IP_12_ADDR   = 0x60000018       
    IP_13_ADDR   = 0x70000018                                       
    IP_14_ADDR   = 0x6000001C                                       
    IP_15_ADDR   = 0x7000001C   
      
    (IP, MAC)    = (0, 1)

    def __init__(self, host, port, timeout, delay, src0addr, src1addr, src2addr, dst0addr, dst1addr, dst2addr):

        self.host     = host #'192.168.0.103'
        self.port     = port # 4321
        self.timeout  = timeout
        self.delay    = delay
        
        if src0addr: self.src0addr = self.extractAddresses(src0addr) 
        else:        self.src0addr = (None, None)
        if src1addr: self.src1addr = self.extractAddresses(src1addr) 
        else:        self.src1addr = (None, None)
        if src2addr: self.src2addr = self.extractAddresses(src2addr) 
        else:        self.src2addr = (None, None)
        if dst0addr: self.dst0addr = self.extractAddresses(dst0addr) 
        else:        self.dst0addr = (None, None)
        if dst1addr: self.dst1addr = self.extractAddresses(dst1addr) 
        else:        self.dst1addr = (None, None)
        if dst2addr: self.dst2addr = self.extractAddresses(dst2addr) 
        else:        self.dst2addr = (None, None)

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

    def extractAddresses(self, jointAddress):
        ''' Extracting mac/IP from parser argument '''
        delim =  jointAddress.find(":")
        ip    = jointAddress[:delim]
        mac   = jointAddress[delim+1:]
        return (ip, mac)

    def execute(self, command):

        self.command = command
        
        if not command in EmulatorClient.LegalCommands:
            raise EmulatorClientError("Illegal command %s specified" % command)
        
        if (command == 'start') or (command == 'stop'):

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
        else:
            
            # Configure link(s) 

            theDelay = self.delay  # Between each TCP transmission
            
            src0mac = self.src0addr[EmulatorClient.MAC]
            src1mac = self.src1addr[EmulatorClient.MAC]
            src2mac = self.src2addr[EmulatorClient.MAC]
            dst0mac = self.dst0addr[EmulatorClient.MAC]
            dst1mac = self.dst1addr[EmulatorClient.MAC]
            dst2mac = self.dst2addr[EmulatorClient.MAC]

            src0ip = self.src0addr[EmulatorClient.IP]
            src1ip = self.src1addr[EmulatorClient.IP]
            src2ip = self.src2addr[EmulatorClient.IP]
            dst0ip = self.dst0addr[EmulatorClient.IP]
            dst1ip = self.dst1addr[EmulatorClient.IP]
            dst2ip = self.dst2addr[EmulatorClient.IP]
            
            print "Sending: "
            if src0ip:
                ipList = self.create_ip(src0ip)
                ipSourceString = ''.join(ipList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_0_ADDR, 4, ipSourceString)
                time.sleep(theDelay)
#
            if dst0ip:
                ipList = self.create_ip(dst0ip)
                ipSourceString = ''.join(ipList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_1_ADDR, 4, ipSourceString)
                time.sleep(theDelay)
                 
            if src1ip:
                ipList = self.create_ip(src1ip)
                ipSourceString = ''.join(ipList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_2_ADDR, 4, ipSourceString)
                time.sleep(theDelay)
#
            if dst1ip:
                ipList = self.create_ip(dst1ip)
                ipSourceString = ''.join(ipList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_3_ADDR, 4, ipSourceString)
                time.sleep(theDelay)
                 
            if src2ip:
                ipList = self.create_ip(src2ip)
                ipSourceString = ''.join(ipList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_4_ADDR, 4, ipSourceString)
                time.sleep(theDelay)
#
            if dst2ip:
                ipList = self.create_ip(dst2ip)
                ipSourceString = ''.join(ipList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.IP_5_ADDR, 4, ipSourceString)
                time.sleep(theDelay)

            # IP before Mac now..
            
            if src0mac:
                tokenList = self.tokeniser(src0mac)
                macSourceStr = ''.join(tokenList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_0_ADDR, 6, macSourceStr)
                time.sleep(theDelay)
#
            if dst0mac:
                tokenList = self.tokeniser(dst0mac)
                macSourceStr = ''.join(tokenList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_1_ADDR, 6, macSourceStr)
                time.sleep(theDelay)
     
            if src1mac:
                tokenList = self.tokeniser(src1mac)
                macSourceStr = ''.join(tokenList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_2_ADDR, 6, macSourceStr)
                time.sleep(theDelay)
#
            if dst1mac:
                tokenList = self.tokeniser(dst1mac)
                macSourceStr = ''.join(tokenList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_3_ADDR, 6, macSourceStr)
                time.sleep(theDelay)
     
            if src2mac:
                tokenList = self.tokeniser(src2mac)
                macSourceStr = ''.join(tokenList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_4_ADDR, 6, macSourceStr)
                time.sleep(theDelay)
#
            if dst2mac:
                tokenList = self.tokeniser(dst2mac)
                macSourceStr = ''.join(tokenList)
                self.send_to_hw(EmulatorClient.Eth_Dev_RW, EmulatorClient.MAC_5_ADDR, 6, macSourceStr)
                time.sleep(theDelay)

            print "\nWaiting 3 seconds before closing TCP connection.."
            time.sleep(3.0)
            print " Done!"

    def tokeniser(self, string):
        """ Remove white spaces and Split (IP/MAC address) string into list """
        string = string.replace(' ', '')
        if ":" in string:
            tokenList = string.split(":")
        else:
            if "." in string:
                tokenList = string.split(".")
            else:
                if "-" in string:
                    tokenList = string.split("-")
                else:
                    tokenList = string.split()
        return tokenList
        
    def create_ip(self, ip_addr):
        ''' Convert IP address from string format into list '''

        ip_value  = ['0'] * 4        # Return value
        int_value = ['0'] * 4
        var_b = ""

        hexString = ""
        var_i = 0
        tokenList = self.tokeniser(ip_addr)
        for index in range(len(tokenList)):
            var_i = 0

            var_i = int(tokenList[index])
            var_b = (var_i & 0x000000FF)

            hexString = hexString + str(tokenList[index] )
            int_value[index] = var_i
            ip_value[index] = '%02X' % (var_b)  # Hexadecimal conversion
        
        return ip_value     # Returns IP address as a list of integers 
    
    
    # Convert create_mac() into Python
    def create_mac(self, mac_add):
        ''' Convert mac address from string format into list '''
        mac_value = ['0'] * 8
        int_value = ['0'] * 8
        var_b    = [0]
        lenToken = 0
        var_i    = 0
        hdata    = ""
        hexString = ""
        tokenList = self.tokeniser(mac_add)
        for index in range(len(tokenList)):
            token = tokenList[index]
            var_i = 0
            #
            var_i = int(tokenList[index], 16)
            lenToken = len(token)
            var_b = (var_i & 0x000000FF)
            
            for k in range(lenToken):
                hdata = token[k]
                var_b = int(hdata, 16) 
                var_i = var_i + var_b*((lenToken-1-k)*16 + k) 
            
            hexString = hexString + token
            int_value[index] = var_i
            mac_value[index] = str(var_b)

        return mac_value 
    
    def intToByte(self, header, offset, length, offset2, command_b):
        ''' Functionality change so that header inserted into command_b according to offset and offset2 '''
        # header is a list of integers, turn them into 8 digits hexadecimals
        for index in range(len(header)):
            hexNum     = "%x" % header[index]
            hexPadded  = (hexNum).zfill(8)
            byteString = ''.join(chr(int(hexPadded[i:i+2], 16)) for i in range(0, len(hexPadded), 2))
            command_b  = command_b[:8+(index*4)] + byteString + command_b[8+((index+1)*4):]
        return command_b
    
    def send_to_hw(self, Dev_RW, ADDR, Length, data):
        ''' Send (IP/Mac address) to Mezzanine '''
        # Example :
        # Device ID  = 0x40000001;  # HW,  Write
        # Address    = 0x18000004;  # Control Register   
        # length     = 0x00000001;  # Control Register    0x0000 0001
        HEADER = [0,0,0,0]
        HEADER[0] = Dev_RW; HEADER[1] = ADDR;  HEADER[2] = Length;
        
        # Extract "command" key from dictionary
        command = EmulatorClient.LegalCommands[self.command] + "0" * 16 # Add room for header + data
        # Copy HEADER into command
        command = self.intToByte( HEADER, 0, 3, 8, command)

        try:
            results =''.join(chr(int(data[i:i+2], 16)) for i in range(0, len(data), 2))
            command = command[:20] +  results
        except Exception as e:
            raise EmulatorClientError("Error manipulating '%s' and '%s', because: '%s'" % (results, command, e))
        
        # Transmit command
        try:
            bytesSent = self.sock.send(command)
        except socket.error, e:
            if self.sock:
                self.sock.close()
            raise EmulatorClientError("Error sending %s command: %s" % (command, e))
        else:
            print " %d bytes.." % (bytesSent),
            sys.stdout.flush()


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="EmulatorClient - control hardware emulator start, stop & configure node(s)", epilog="Specify IP & Mac like: '10.1.0.101:00-07-11-F0-FF-33'")
    
    parser.add_argument('--host', type=str, default='192.168.0.103', 
                        help="select emulator IP address")
    parser.add_argument('--port', type=int, default=4321,
                        help='select emulator IP port')
    parser.add_argument('--timeout', type=int, default=5,
                        help='set TCP connection timeout')
    parser.add_argument ('--delay', type=float, default=1.00,
                         help='set delay between each address packet')

    parser.add_argument('--src0', type=str, 
                        help='Configure Mezzanine link 0 IP:MAC addresses')
    parser.add_argument('--src1', type=str, 
                        help='Configure Mezzanine link 1 IP:MAC addresses')
    parser.add_argument('--src2', type=str, 
                        help='Configure Mezzanine link 2 IP:MAC addresses')
    
    parser.add_argument('--dst0', type=str, 
                        help='Configure PC link 0 IP:MAC addresses')
    parser.add_argument('--dst1', type=str, 
                        help='Configure PC link 1 IP:MAC addresses')
    parser.add_argument('--dst2', type=str, 
                        help='Configure PC link 2 IP:MAC addresses')
    
    parser.add_argument('command', choices=EmulatorClient.LegalCommands.keys(), default="start",
                        help="specify which command to send to emulator")

    args = parser.parse_args()
    try:
        #print "args: ", args, args.__dict__    # Display all parser selection(s)
        client = EmulatorClient(args.host, args.port, args.timeout, args.delay, args.src0, args.src1, args.src2, args.dst0, args.dst1, args.dst2)
        #    def __init__(self, host, port, timeout, delay, src0addr, src1addr, src2addr, dst0addr, dst1addr, dst2addr):
        client.execute(args.command)
    except Exception as e:
        print e

