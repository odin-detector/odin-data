import posix_ipc
import mmap
import ctypes
import os
import errno
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
    boost_mmap_path = '/tmp/boost_interprocess'


    def __init__(self, shared_mem_name, shared_mem_size=0, buffer_size=0,
                 remove_when_deleted=False, boost_mmap_mode=False):

        self.remove_when_deleted = remove_when_deleted
        self.shared_mem = None
        self.mmap_file = None
        self.mapfile = None

        if shared_mem_size:
            total_size = shared_mem_size + SharedBufferManager.Header.size
            shm_flags = posix_ipc.O_CREX
            mmap_file_mode = 'w+b'
        else:
            total_size = 0
            shm_flags = 0
            mmap_file_mode = 'r+b'

        if boost_mmap_mode:

            # Create the boost mmap file directory if it doesn't exist alread
            try:
                os.makedirs(SharedBufferManager.boost_mmap_path)
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise SharedBufferManagerException(str(e))

            shared_mem_name = os.path.join(SharedBufferManager.boost_mmap_path, shared_mem_name)

            if shared_mem_size and os.path.exists(shared_mem_name):
                raise SharedBufferManagerException("Shared memory with the specified name already exists")

            try:
                self.mmap_file = open(shared_mem_name, mmap_file_mode)
            except IOError as e:
                if e.errno == 2:
                    raise SharedBufferManagerException("No shared memory exists with the specified name")
                else:
                    raise SharedBufferManagerException(str(e))

            if shared_mem_size:
                self.mmap_file.truncate(total_size)
            mmap_size = 0
            mmap_fd = self.mmap_file.fileno()

        else:
            try:
                self.shared_mem = posix_ipc.SharedMemory(
                    shared_mem_name, flags=shm_flags, mode=0o755, size=total_size)
                mmap_size = self.shared_mem.size
                mmap_fd = self.shared_mem.fd
            except posix_ipc.ExistentialError as e:
                raise SharedBufferManagerException(str(e))
            except posix_ipc.Error  as e:
                raise SharedBufferManagerException(str(e))
            except ValueError as e:
                raise SharedBufferManagerException(str(e))

        self.mapfile = mmap.mmap(mmap_fd, mmap_size, access=mmap.ACCESS_WRITE)


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

        if self.mapfile:
            self.mapfile.close()

        if self.mmap_file:
            self.mmap_file.close()
            if self.remove_when_deleted:
                os.remove(self.mmap_file.name)

        if self.shared_mem:
            self.shared_mem.close_fd()
            if self.remove_when_deleted:
                self.shared_mem.unlink()
