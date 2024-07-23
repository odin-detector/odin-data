import os
import time

import h5py as h5
import numpy

from odin_data.meta_writer.hdf5dataset import (
    HDF5UnlimitedCache,
    StringHDF5Dataset,
    Int32HDF5Dataset
)
import tempfile


class _TestMockDataset:
    def __init__(self):
        self.size = -1
        self.axis = -1
        self.keys = []
        self.values = []

    def resize(self, size, axis):
        self.size = size
        self.axis = axis

    def len(self):
        return self.size

    def __setitem__(self, key, value):
        self.keys.append(key)
        self.values.append(value)


class TestHDF5Dataset:
    def test_unlimited_cache_1D(self):
        """verify that the cache functions as expected"""

        # Create a cache
        cache = HDF5UnlimitedCache(
            name="test1",
            dtype="int32",
            fillvalue=-2,
            block_size=10,
            data_shape=(1,),
            block_timeout=0.01,
        )

        # Verify 1 block has been created, with a size of 10
        assert len(cache._blocks) == 1
        assert 0 in cache._blocks
        assert cache._blocks[0].has_new_data
        assert cache._blocks[0].data.size == 10

        # Add 11 items at increasing indexes and verify another
        # block is created
        for offset in range(11):
            cache.add_value(2, offset)

        assert len(cache._blocks) == 2
        assert 0 in cache._blocks
        assert 1 in cache._blocks
        assert cache._blocks[0].has_new_data
        assert cache._blocks[0].data.size == 10
        assert sum(cache._blocks[0].data) == 20
        assert cache._blocks[1].has_new_data
        assert cache._blocks[1].data.size == 10

        # Add 1 item at index to create another block
        cache.add_value(3, 25)

        assert len(cache._blocks) == 3
        assert 0 in cache._blocks
        assert 1 in cache._blocks
        assert 2 in cache._blocks
        assert cache._blocks[0].has_new_data
        assert cache._blocks[0].data.size, 10
        assert sum(cache._blocks[0].data), 20
        assert cache._blocks[1].has_new_data
        assert cache._blocks[1].data.size, 10
        assert sum(cache._blocks[1].data), -16  # 2 + (9 * -2) fillvalue
        assert cache._blocks[2].has_new_data
        assert cache._blocks[2].data.size, 10
        assert sum(cache._blocks[2].data), -15  # 3 + (9 * -2) fillvalue

        # Now flush the blocks
        ds = _TestMockDataset()
        cache.flush(ds)

        # Verify the flushed slices are expected (2 full blocks and the last partial)
        assert ds.keys == [slice(0, 10, None), slice(10, 20, None), slice(20, 26, None)]
        assert ds.values[0][0] == 2
        assert ds.values[0][1] == 2
        assert ds.values[0][2] == 2
        assert ds.values[0][3] == 2
        assert ds.values[0][4] == 2
        assert ds.values[0][5] == 2
        assert ds.values[0][6] == 2
        assert ds.values[0][7] == 2
        assert ds.values[0][8] == 2
        assert ds.values[0][9] == 2
        assert ds.values[1][0] == 2
        assert ds.values[1][1] == -2
        assert ds.values[1][2] == -2
        assert ds.values[1][3] == -2
        assert ds.values[1][4] == -2
        assert ds.values[1][5] == -2
        assert ds.values[1][6] == -2
        assert ds.values[1][7] == -2
        assert ds.values[1][8] == -2
        assert ds.values[1][9] == -2

        # Now wait
        time.sleep(0.1)

        # Now write another value into block[2] and then flush once again
        cache.add_value(3, 26)

        # Now flush again and then purge, block [0] and [1] should be deleted
        ds = _TestMockDataset()
        cache.flush(ds)
        cache.purge_blocks()

        assert len(cache._blocks) == 1
        assert 0 not in cache._blocks
        assert 1 not in cache._blocks
        assert 2 in cache._blocks
        assert not cache._blocks[2].has_new_data
        assert cache._blocks[2].data.size == 10
        assert sum(cache._blocks[2].data) == -10  # 6 + (8 * -2) fillvalue

        assert ds.keys == [slice(20, 27, None)]
        assert ds.values[0][0] == -2
        assert ds.values[0][1] == -2
        assert ds.values[0][2] == -2
        assert ds.values[0][3] == -2
        assert ds.values[0][4] == -2
        assert ds.values[0][5] == 3
        assert ds.values[0][6] == 3

    def test_unlimited_cache_2D(self):
        """verify that the cache functions as expected"""

        # Create a cache
        cache = HDF5UnlimitedCache(
            name="test1",
            dtype="int32",
            fillvalue=-2,
            block_size=10,
            data_shape=(2, 3),
            block_timeout=0.01,
        )

        # Verify 1 block has been created, with a size of 10x2x3
        assert len(cache._blocks) == 1
        assert 0 in cache._blocks
        assert cache._blocks[0].has_new_data
        assert cache._blocks[0].data.size == 60

        # Add 11 items at increasing indexes and verify another
        # block is created
        value = [[1, 2, 3], [4, 5, 6]]
        for offset in range(11):
            cache.add_value(value, offset)

        assert len(cache._blocks) == 2
        assert 0 in cache._blocks
        assert 1 in cache._blocks
        assert cache._blocks[0].has_new_data
        assert cache._blocks[0].data.size == 60
        assert numpy.sum(cache._blocks[0].data) == 210
        assert cache._blocks[1].has_new_data
        assert cache._blocks[1].data.size == 60

        # Add 1 item at index to create another block
        cache.add_value(value, 25)

        assert len(cache._blocks) == 3
        assert 0 in cache._blocks
        assert 1 in cache._blocks
        assert 2 in cache._blocks
        assert cache._blocks[0].has_new_data
        assert cache._blocks[0].data.size == 60
        assert numpy.sum(cache._blocks[0].data) == 210
        assert cache._blocks[1].has_new_data
        assert cache._blocks[1].data.size == 60
        assert numpy.sum(cache._blocks[1].data) == -87  # 21 + (9 * -12) fillvalue
        assert cache._blocks[2].has_new_data
        assert cache._blocks[2].data.size == 60
        assert numpy.sum(cache._blocks[2].data) == -87  # 21 + (9 * -12) fillvalue

        # Now flush the blocks
        ds = _TestMockDataset()
        cache.flush(ds)

        # Verify the flushed slices are expected (2 full blocks and the last partial)
        assert ds.keys == [slice(0, 10, None), slice(10, 20, None), slice(20, 26, None)]
        assert ds.values[0][0][0][0] == 1
        assert ds.values[0][0][0][1] == 2
        assert ds.values[0][0][0][2] == 3
        assert ds.values[0][0][1][0] == 4
        assert ds.values[0][0][1][1] == 5
        assert ds.values[0][0][1][2] == 6
        assert ds.values[0][5][0][0] == 1
        assert ds.values[0][5][0][1] == 2
        assert ds.values[0][5][0][2] == 3
        assert ds.values[0][5][1][0] == 4
        assert ds.values[0][5][1][1] == 5
        assert ds.values[0][1][1][2] == 6
        assert ds.values[1][1][0][0] == -2
        assert ds.values[1][1][0][1] == -2
        assert ds.values[1][1][0][2] == -2
        assert ds.values[1][1][1][0] == -2
        assert ds.values[1][1][1][1] == -2
        assert ds.values[1][1][1][2] == -2

        # Now wait
        time.sleep(0.1)

        # Now write another value into block[2] and then flush once again
        cache.add_value(value, 26)

        # Now flush again and purge, block [0] and [1] should be deleted
        ds = _TestMockDataset()
        cache.flush(ds)
        cache.purge_blocks()

        assert len(cache._blocks) == 1
        assert 0 not in cache._blocks
        assert 1 not in cache._blocks
        assert 2 in cache._blocks
        assert not cache._blocks[2].has_new_data
        assert cache._blocks[2].data.size == 60
        assert numpy.sum(cache._blocks[2].data) == -54  # 42 + (8 * -12) fillvalue

        assert ds.keys == [slice(20, 27, None)]
        assert ds.values[0][0][0][0] == -2
        assert ds.values[0][0][0][1] == -2
        assert ds.values[0][0][0][2] == -2
        assert ds.values[0][0][1][0] == -2
        assert ds.values[0][0][1][1] == -2
        assert ds.values[0][0][1][2] == -2

        assert ds.values[0][5][0][0] == 1
        assert ds.values[0][5][0][1] == 2
        assert ds.values[0][5][0][2] == 3
        assert ds.values[0][5][1][0] == 4
        assert ds.values[0][5][1][1] == 5
        assert ds.values[0][5][1][2] == 6

        assert ds.values[0][6][0][0] == 1
        assert ds.values[0][6][0][1] == 2
        assert ds.values[0][6][0][2] == 3
        assert ds.values[0][6][1][0] == 4
        assert ds.values[0][6][1][1] == 5
        assert ds.values[0][6][1][2] == 6


def test_string_types():
    variable_utf = StringHDF5Dataset("variable_utf", encoding="utf-8", cache=False)
    variable_ascii = StringHDF5Dataset("variable_ascii", encoding="ascii", cache=False)
    fixed_utf = StringHDF5Dataset("fixed_utf", encoding="utf-8", length=9, cache=False)
    fixed_ascii = StringHDF5Dataset(
        "fixed_ascii", encoding="ascii", length=11, cache=False
    )
    variable_utf_int = StringHDF5Dataset(
        "variable_utf_int", encoding="utf-8", cache=False
    )

    temp_file = tempfile.TemporaryFile()
    with h5.File(temp_file, "w") as f:
        variable_utf.initialise(
            f.create_dataset(
                "variable_utf", shape=(1,), maxshape=(1,), dtype=variable_utf.dtype
            ),
            0,
        )
        variable_ascii.initialise(
            f.create_dataset(
                "variable_ascii", shape=(1,), maxshape=(1,), dtype=variable_ascii.dtype
            ),
            0,
        )
        fixed_utf.initialise(
            f.create_dataset(
                "fixed_utf", shape=(1,), maxshape=(1,), dtype=fixed_utf.dtype
            ),
            0,
        )
        fixed_ascii.initialise(
            f.create_dataset(
                "fixed_ascii", shape=(1,), maxshape=(1,), dtype=fixed_ascii.dtype
            ),
            0,
        )
        variable_utf_int.initialise(
            f.create_dataset(
                "variable_utf_int", shape=(1,), maxshape=(1,), dtype=variable_utf.dtype
            ),
            0,
        )

        variable_utf.write("variable_utf")
        variable_ascii.write("variable_ascii")
        fixed_utf.write("fixed_utf")
        fixed_ascii.write("fixed_ascii")
        variable_utf_int.write(2)  # Check non-strings can be handled

    with h5.File(temp_file, "r") as f:
        assert f["variable_utf"][0] == b"variable_utf"
        assert f["variable_ascii"][0] == b"variable_ascii"
        assert f["fixed_utf"][0] == b"fixed_utf"
        assert f["fixed_ascii"][0] == b"fixed_ascii"
        assert f["variable_utf_int"][0] == b"2"


def test_attributes_added():
    dataset_with_units = Int32HDF5Dataset("test_dataset", cache=False, attributes={"units": "m"})

    temp_file = tempfile.TemporaryFile()
    with h5.File(temp_file, "w") as f:
        dataset_with_units.initialise(
            f.create_dataset(
                "test_dataset", shape=(1,), maxshape=(1,), dtype=dataset_with_units.dtype
            ),
            0,
        )

        dataset_with_units.write(2)

    with h5.File(temp_file, "r") as f:
        attributes = f["test_dataset"].attrs
        assert len(attributes.values()) == 1
        assert attributes["units"] == "m"