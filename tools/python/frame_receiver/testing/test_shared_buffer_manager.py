from frame_receiver.shared_buffer_manager import SharedBufferManager, SharedBufferManagerException
from nose.tools import assert_equal, assert_raises, assert_regexp_matches
from struct import Struct

shared_mem_name = "TestSharedBuffer"
buffer_size     = 1000
num_buffers     = 10
shared_mem_size = buffer_size * num_buffers
boost_mmap_mode = True

class TestSharedBufferManager:
    
    @classmethod
    def setup_class(cls):
        
        # Create a shared buffer manager for use in all tests
        cls.shared_buffer_manager = SharedBufferManager(
                shared_mem_name, shared_mem_size, 
                buffer_size, remove_when_deleted=True, boost_mmap_mode=boost_mmap_mode)
        
    @classmethod
    def teardown_class(cls):
        
        pass
                
    def test_basic_shared_buffer(self):

        # Test the shared buffer manager confoguration is expected        
        assert_equal(self.shared_buffer_manager.get_num_buffers(), num_buffers)
        assert_equal(self.shared_buffer_manager.get_buffer_size(), buffer_size)
        
    def test_existing_manager(self):
        
        # Map the existing manager
        existing_shared_buffer = SharedBufferManager(shared_mem_name, boost_mmap_mode=boost_mmap_mode)
            
        # Test that the configuration matches the original
        assert_equal(self.shared_buffer_manager.get_manager_id(), existing_shared_buffer.get_manager_id())
        assert_equal(self.shared_buffer_manager.get_num_buffers(), existing_shared_buffer.get_num_buffers())
        assert_equal(self.shared_buffer_manager.get_buffer_size(), existing_shared_buffer.get_buffer_size())
        
    def test_existing_manager_absent(self):
        
        # Attempt to map a shared buffer manager that doesn't already exist
        absent_manager_name = "AbsentBufferManager"
        with assert_raises(SharedBufferManagerException) as cm:
            existing_shared_buffer = SharedBufferManager(absent_manager_name, boost_mmap_mode=boost_mmap_mode)
        ex = cm.exception
        assert_regexp_matches(ex.msg, "No shared memory exists with the specified name")
        
    def test_existing_manager_already_exists(self):
        
        # Attempt to create a shared buffer manager that already exists
        with assert_raises(SharedBufferManagerException) as cm:
            clobbered_shared_buffer = SharedBufferManager(shared_mem_name, 
                                                          100, 100, True, boost_mmap_mode=boost_mmap_mode)
        ex = cm.exception
        assert_regexp_matches(ex.msg, "Shared memory with the specified name already exists")
        
    def test_illegal_shared_buffer_index(self):

        with assert_raises(SharedBufferManagerException) as cm:
            buffer_address = self.shared_buffer_manager.get_buffer_address(-1)
        ex = cm.exception
        assert_regexp_matches(ex.msg, "Illegal buffer index specified")
                
        with assert_raises(SharedBufferManagerException) as cm:
            buffer_address = self.shared_buffer_manager.get_buffer_address(num_buffers)
        ex = cm.exception
        assert_regexp_matches(ex.msg, "Illegal buffer index specified")
        
    def test_write_and_read_from_buffer(self):
        
        data_block = Struct('QQQ')
        
        values = (0xdeadbeef, 0x12345678, 0xaaaa5555aaaa5555)
        raw_data = data_block.pack(*values)
        self.shared_buffer_manager.write_buffer(0, raw_data)
        
        read_raw = self.shared_buffer_manager.read_buffer(0, data_block.size)
        read_values = data_block.unpack(read_raw)
        
        assert_equal(values, read_values)