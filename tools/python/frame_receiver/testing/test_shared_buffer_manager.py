from frame_receiver.shared_buffer_manager import SharedBufferManager, SharedBufferManagerException
from nose.tools import assert_equal, assert_raises, assert_regexp_matches
from struct import Struct

shared_mem_name = "TestSharedBuffer"
buffer_size     = 1000
num_buffers     = 10
shared_mem_size = buffer_size * num_buffers

class TestSharedBufferManager:
    
    @classmethod
    def setup_class(cls):
        
        cls.shared_buffer_manager = SharedBufferManager(shared_mem_name, shared_mem_size, buffer_size)
        print "Set up class"
        
    @classmethod
    def teardown_class(cls):
        
        print "Tear down class"
        
    def test_basic_shared_buffer(self):
        
        assert_equal(self.shared_buffer_manager.get_num_buffers(), num_buffers)
        assert_equal(self.shared_buffer_manager.get_buffer_size(), buffer_size)
        
        
    def test_existing_manager(self):
        
        existing_shared_buffer = SharedBufferManager(shared_mem_name)
        assert_equal(self.shared_buffer_manager.get_manager_id(), existing_shared_buffer.get_manager_id())
        assert_equal(self.shared_buffer_manager.get_num_buffers(), existing_shared_buffer.get_num_buffers())
        assert_equal(self.shared_buffer_manager.get_buffer_size(), existing_shared_buffer.get_buffer_size())
        
        
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