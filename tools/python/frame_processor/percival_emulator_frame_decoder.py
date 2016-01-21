from datetime import datetime
from struct import calcsize, Struct

class PercivalFrameHeader(Struct):
    
    frame_header_format = '<LLQQL14B1024B6B'
    
    @classmethod
    def size(cls):       
        return calcsize(PercivalFrameHeader.frame_header_format)
    
    def __init__(self, header_raw):
        
        super(PercivalFrameHeader, self).__init__(PercivalFrameHeader.frame_header_format)
        
        header_vals = self.unpack(header_raw)

        self.frame_number = header_vals[0]
        self.frame_state = header_vals[1]
        self.frame_start_time = datetime.fromtimestamp(float(header_vals[2]) + float(header_vals[3])/1000000000)
        self.packets_received = header_vals[4]
        self.frame_info = header_vals[5:19]
        self.packet_state = header_vals[19:-6]
        self.padding = header_vals[-6:]
        

class PercivalFrameData(Struct):
    
    primary_packet_size = 8192
    num_primary_packets = 255
    tail_packet_size    = 512
    num_tail_packets    = 1
    num_subframes       = 2
    num_data_types      = 2
 
    subframe_size = (num_primary_packets * primary_packet_size) + (num_tail_packets * tail_packet_size)
    data_type_size = subframe_size * num_subframes
    total_data_size = data_type_size * num_data_types
    
    pixel_rows = 1484
    pixel_cols = 1408
    num_pixels = pixel_rows * pixel_cols * num_data_types
    
    frame_data_format = '<' + str(num_pixels) + 'H'
    
    @classmethod
    def size(cls):
        return calcsize(PercivalFrameData.frame_data_format)
    
    def __init__(self, data_raw):
        
        super(PercivalFrameData, self).__init__(PercivalFrameData.frame_data_format)
        self.pixels = self.unpack(data_raw)
        
class PercivalEmulatorFrameDecoder(object):
       
    def __init__(self, shared_buffer_manager):
        
        self.shared_buffer_manager = shared_buffer_manager
    
    def decode_header(self, buffer_id):
        
        header_raw = self.shared_buffer_manager.read_buffer(buffer_id, PercivalFrameHeader.size())
        self.header = PercivalFrameHeader(header_raw)
        
    def decode_data(self, buffer_id):
        
        data_raw = self.shared_buffer_manager.read_buffer(buffer_id, PercivalFrameData.size(), PercivalFrameHeader.size())
        self.data = PercivalFrameData(data_raw) 