from datetime import datetime
from struct import calcsize, Struct

p2m_emulator_new_firmware = True

class PercivalFrameHeader(Struct):
    
    if p2m_emulator_new_firmware:
        frame_header_format = '<LLQQL42B1696B2B'
    else:
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
        if p2m_emulator_new_firmware:
            self.frame_info = header_vals[5:47]
            self.packet_state = header_vals[47:-2]
            self.padding = header_vals[-2:]
        else:
            self.frame_info = header_vals[5:19]
            self.packet_state = header_vals[19:-6]
            self.padding = header_vals[-6:]
        

class PercivalFrameData(Struct):
    
    if p2m_emulator_new_firmware:
        primary_packet_size = 4928
        num_primary_packets = 424
        tail_packet_size    = 0
        num_tail_packets    = 0
        num_subframes       = 2
        num_data_types      = 2
    else:
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
        self.header = None
    
    def decode_header(self, buffer_id):
        
        header_raw = self.shared_buffer_manager.read_buffer(buffer_id, PercivalFrameHeader.size())
        self.header = PercivalFrameHeader(header_raw)
        
    def packet_state_str(self, step=32):
        
        packet_state_str = ""

        step_format = '| {{:{:d}}} '.format(step)
        
        num_packets = PercivalFrameData.num_primary_packets + PercivalFrameData.num_tail_packets
 
        if self.header is not None:

            for data_type in range(PercivalFrameData.num_data_types):
                for subframe in range(PercivalFrameData.num_subframes):
                    type_str = "Reset" if data_type == 1 else "Image"
                    packet_state_str += step_format.format('Subframe: {} {}'.format(subframe, type_str))
            packet_state_str += "|\n"

            offset = 0
 
            while offset < num_packets:
                for data_type in range(PercivalFrameData.num_data_types):
                    for subframe in range(PercivalFrameData.num_subframes):
                       start_idx = (subframe * num_packets) + (data_type * (PercivalFrameData.num_subframes * num_packets)) + offset
                       end_idx = start_idx + step
                       if (offset + step) > num_packets:
                           end_idx -= ((offset + step) - num_packets)
                       packet_state_str += step_format.format(''.join('{:1}'.format('.' if val == 0 else '*') for val in self.header.packet_state[start_idx:end_idx]))
                packet_state_str += "|\n"
                offset += step
           
        return packet_state_str
        
    def decode_data(self, buffer_id):
        
        data_raw = self.shared_buffer_manager.read_buffer(buffer_id, PercivalFrameData.size(), PercivalFrameHeader.size())
        self.data = PercivalFrameData(data_raw) 