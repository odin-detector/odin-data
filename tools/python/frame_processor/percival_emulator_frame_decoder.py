from datetime import datetime
from struct import calcsize, Struct

class PercivalFrameHeader(Struct):
    
    FrameHeaderFormat = '<LLQQL'
    
    @classmethod
    def header_size(cls):       
        return calcsize(PercivalFrameHeader.FrameHeaderFormat)
    
    def __init__(self, header_raw):
        
        super(PercivalFrameHeader, self).__init__(PercivalFrameHeader.FrameHeaderFormat)
        
        header_vals = self.unpack(header_raw)

        self.frame_number = header_vals[0]
        self.frame_state = header_vals[1]
        self.frame_start_time = datetime.fromtimestamp(float(header_vals[2]) + float(header_vals[3])/1000000000)
        self.packets_received = header_vals[4]
        
class PercvialEmulatorFrameDecoder(object):
       
    def __init__(self, shared_buffer_manager):
        
        self.shared_buffer_manager = shared_buffer_manager
    
    def decode_header(self, buffer_id):
        
        header_raw = self.shared_buffer_manager.read_buffer(buffer_id, PercivalFrameHeader.header_size())
        self.header = PercivalFrameHeader(header_raw)