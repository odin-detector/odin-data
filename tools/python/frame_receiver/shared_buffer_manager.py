import posix_ipc
import mmap
import ctypes
from struct import Struct

class SharedBufferManagerException(Exception):
    
    def __init__(self, msg, errno=None):
        self.msg = msg
        self.errno = errno
    
    def __str__(self):
        return str(self.msg)
    
class SharedBufferManager(object):
    
    Header = Struct('QQQ')
    _last_manager_id = 0x100
    
    def __init__(self, shared_mem_name, shared_mem_size=0, buffer_size=0, remove_when_deleted=False):
        
        if shared_mem_size:
            total_size = shared_mem_size + SharedBufferManager.Header.size
        else:
            total_size = 0
            
        self.shared_mem = posix_ipc.SharedMemory(shared_mem_name, flags=posix_ipc.O_CREAT, mode=0755, size=total_size)
        self.mapfile = mmap.mmap(self.shared_mem.fd, self.shared_mem.size)
        self.remove_when_deleted = remove_when_deleted

        self.manager_id = ctypes.c_int64.from_buffer(self.mapfile)
        self.num_buffers = ctypes.c_int64.from_buffer(self.mapfile, 8)
        self.buffer_size = ctypes.c_int64.from_buffer(self.mapfile, 16)
        
        if shared_mem_size:
                       
            self.manager_id.value = self.__class__._last_manager_id
            self.__class__._last_manager_id += 1
            self.num_buffers.value = int(shared_mem_size / buffer_size)
            self.buffer_size.value = buffer_size
            
            
        self.mapfile.seek(0)
        
    def get_manager_id(self):
        
        return self.manager_id.value
    
    def get_num_buffers(self):
        
        return self.num_buffers.value
    
    def get_buffer_size(self):
        
        return self.buffer_size.value
        
    def get_buffer_address(self, buffer_index):
        
        if buffer_index < 0 or buffer_index >= self.num_buffers.value:
            raise SharedBufferManagerException("Illegal buffer index specified: " + str(buffer_index))
        
        return SharedBufferManager.Header.size + (self.buffer_size.value * buffer_index)
    
    def read_buffer(self, buffer_index, num_bytes=1, offset=0):
        
        buf_addr = self.get_buffer_address(buffer_index)
        cur_pos = self.mapfile.tell()
        start_addr = buf_addr + offset
        
        self.mapfile.seek(start_addr)
        data = self.mapfile.read(num_bytes)
        self.mapfile.seek(cur_pos)
        
        return data
        
    def write_buffer(self, buffer_index, data, offset=0):
        
        buf_addr = self.get_buffer_address(buffer_index)
        cur_pos = self.mapfile.tell()
        start_addr = buf_addr + offset
        self.mapfile.seek(start_addr)
        self.mapfile.write(data)
        self.mapfile.seek(cur_pos)
    
    def __del__(self):

        self.mapfile.close()
        self.shared_mem.close_fd()
               
        if self.remove_when_deleted:
            self.shared_mem.unlink()