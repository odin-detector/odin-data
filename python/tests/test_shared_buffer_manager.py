from struct import Struct
import pytest
from odin_data.shared_buffer_manager import SharedBufferManager, SharedBufferManagerException


shared_mem_name = "TestSharedBuffer"
buffer_size = 1000
num_buffers = 10
shared_mem_size = buffer_size * num_buffers
boost_mmap_mode = True


@pytest.fixture(scope="class")
def shared_buffer_manager():
    return SharedBufferManager(
        shared_mem_name,
        shared_mem_size,
        buffer_size,
        remove_when_deleted=True,
        boost_mmap_mode=boost_mmap_mode,
    )


class TestSharedBufferManager:
    def test_basic_shared_buffer(self, shared_buffer_manager):
        # Test the shared buffer manager confoguration is expected
        assert shared_buffer_manager.get_num_buffers() == num_buffers
        assert shared_buffer_manager.get_buffer_size() == buffer_size

    def test_existing_manager(self, shared_buffer_manager):
        # Map the existing manager
        existing_shared_buffer = SharedBufferManager(
            shared_mem_name, boost_mmap_mode=boost_mmap_mode
        )

        # Test that the configuration matches the original
        assert shared_buffer_manager.get_manager_id() == existing_shared_buffer.get_manager_id()
        assert shared_buffer_manager.get_num_buffers() == existing_shared_buffer.get_num_buffers()
        assert shared_buffer_manager.get_buffer_size() == existing_shared_buffer.get_buffer_size()

    def test_existing_manager_absent(self):
        # Attempt to map a shared buffer manager that doesn't already exist
        absent_manager_name = "AbsentBufferManager"
        with pytest.raises(SharedBufferManagerException) as excinfo:
            existing_shared_buffer = SharedBufferManager(
                absent_manager_name, boost_mmap_mode=boost_mmap_mode
            )
        assert "No shared memory exists with the specified name" in str(excinfo.value)

    def test_existing_manager_already_exists(self):
        # Attempt to create a shared buffer manager that already exists
        with pytest.raises(SharedBufferManagerException) as excinfo:
            clobbered_shared_buffer = SharedBufferManager(
                shared_mem_name, 100, 100, True, boost_mmap_mode=boost_mmap_mode
            )
        assert "Shared memory with the specified name already exists" in str(excinfo.value)

    def test_illegal_shared_buffer_index(self, shared_buffer_manager):
        with pytest.raises(SharedBufferManagerException) as excinfo:
            buffer_address = shared_buffer_manager.get_buffer_address(-1)
        assert "Illegal buffer index specified" in str(excinfo.value)

        with pytest.raises(SharedBufferManagerException) as excinfo:
            buffer_address = shared_buffer_manager.get_buffer_address(num_buffers)
        assert "Illegal buffer index specified" in str(excinfo.value)

    def test_write_and_read_from_buffer(self, shared_buffer_manager):
        data_block = Struct("QQQ")

        values = (0xDEADBEEF, 0x12345678, 0xAAAA5555AAAA5555)
        raw_data = data_block.pack(*values)
        shared_buffer_manager.write_buffer(0, raw_data)

        read_raw = shared_buffer_manager.read_buffer(0, data_block.size)
        read_values = data_block.unpack(read_raw)

        assert values == read_values
