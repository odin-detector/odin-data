from datetime import datetime
from struct import calcsize, Struct

class ExcaliburFrameHeader(Struct):
    
    frame_header_format = '<LLQQLBB132B6B'
    
    @classmethod
    def size(cls):       
        return calcsize(ExcaliburFrameHeader.frame_header_format)
    
    def __init__(self, header_raw):
        
        super(ExcaliburFrameHeader, self).__init__(ExcaliburFrameHeader.frame_header_format)
        
        header_vals = self.unpack(header_raw)

        self.frame_number = header_vals[0]
        self.frame_state = header_vals[1]
        self.frame_start_time = datetime.fromtimestamp(float(header_vals[2]) + float(header_vals[3])/1000000000)
        self.packets_received = header_vals[4]
        self.sof_marker_count = header_vals[5]
        self.eof_marker_count = header_vals[6]
        self.packet_state = header_vals[7:-6]
        self.padding = header_vals[-6:]
        

class ExcaliburFrameData(Struct):
    
    primary_packet_size = 8000
    num_primary_packets = 65
    tail_packet_size    = 4926
    num_tail_packets    = 1
    num_subframes       = 2
 
    subframe_size = (num_primary_packets * primary_packet_size) + (num_tail_packets * tail_packet_size)
    total_data_size = subframe_size * num_subframes
    
    pixel_rows = 256
    pixel_cols = 2048
    pixels_per_subframe = (pixel_rows * pixel_cols) / 2
    subframe_pixel_format = str(pixels_per_subframe) + 'H'
    subframe_trailer_format = 'Q'
    subframe_format = subframe_pixel_format + subframe_trailer_format
    
    frame_data_format = '<' + subframe_format * 2
    
    @classmethod
    def size(cls):
        return calcsize(ExcaliburFrameData.frame_data_format)
    
    def __init__(self, data_raw):
        
        super(ExcaliburFrameData, self).__init__(ExcaliburFrameData.frame_data_format)
        self.pixels = self.unpack(data_raw)
        
class ExcaliburFrameDecoder(object):
       
    def __init__(self, shared_buffer_manager):
        
        self.shared_buffer_manager = shared_buffer_manager
        self.header = None
    
    def decode_header(self, buffer_id):
        
        header_raw = self.shared_buffer_manager.read_buffer(buffer_id, ExcaliburFrameHeader.size())
        self.header = ExcaliburFrameHeader(header_raw)
        
    def header_state_str(self):
        
        header_state_str = ''
        
        if self.header is not None:

            header_state_str += '      Frame number {:d} State {:d} Start time {:s} Packets received {:d} '.format(
                self.header.frame_number, self.header.frame_state, self.header.frame_start_time.isoformat(),
                self.header.packets_received
            )
            header_state_str += 'SOF marker count {:d} EOF marker count {:d}'.format(
                self.header.sof_marker_count, self.header.eof_marker_count
            )
                                                            
        return header_state_str
        
        
    def packet_state_str(self, step=32):
        
        packet_state_str = ""

        step_format = '| {{:{:d}}} '.format(step)
        
        num_packets = ExcaliburFrameData.num_primary_packets + ExcaliburFrameData.num_tail_packets
 
        if self.header is not None:

            packet_state_str += '      '
            for subframe in range(ExcaliburFrameData.num_subframes):
                packet_state_str += step_format.format('Subframe: {}'.format(subframe))
            packet_state_str += "|\n      "

            offset = 0
 
            while offset < num_packets:
                for subframe in range(ExcaliburFrameData.num_subframes):
                   start_idx = (subframe * num_packets) +  + offset
                   end_idx = start_idx + step
                   if (offset + step) > num_packets:
                       end_idx -= ((offset + step) - num_packets)
                   packet_state_str += step_format.format(''.join('{:1}'.format('.' if val == 0 else '*') for val in self.header.packet_state[start_idx:end_idx]))
                packet_state_str += "|\n      "
                offset += step
           
        return packet_state_str
        
    def decode_data(self, buffer_id):
        
        data_raw = self.shared_buffer_manager.read_buffer(buffer_id, ExcaliburFrameData.size(), ExcaliburFrameHeader.size())
        self.data = ExcaliburFrameData(data_raw) 