
from unittest import TestCase
import logging
import pytest
import numpy
import os
import time

from hdf5dataset import HDF5UnlimitedCache

class _TestMockDataset():
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


class TestHDF5Dataset(TestCase):
    def test_unlimited_cache_1D(self):
        """ verify that the cache functions as expected
        """

        # Create a cache
        cache = HDF5UnlimitedCache('test1', 'int32', -2, 10, 1)

        # Verify 1 block has been created, with a size of 10
        self.assertEqual(len(cache._blocks), 1)
        self.assertEqual(0 in cache._blocks, True)
        self.assertEqual(cache._blocks[0]['active'], True)
        self.assertEqual(cache._blocks[0]['data'].size, 10)

        # Add 11 items at increasing indexes and verify another
        # block is created
        for offset in range(11):
            cache.add_value(2, offset)

        self.assertEqual(len(cache._blocks), 2)
        self.assertEqual(0 in cache._blocks, True)
        self.assertEqual(1 in cache._blocks, True)
        self.assertEqual(cache._blocks[0]['active'], True)
        self.assertEqual(cache._blocks[0]['data'].size, 10)
        self.assertEqual(sum(cache._blocks[0]['data']), 20)
        self.assertEqual(cache._blocks[1]['active'], True)
        self.assertEqual(cache._blocks[1]['data'].size, 10)

        # Add 1 item at index to create another block
        cache.add_value(3, 25)

        self.assertEqual(len(cache._blocks), 3)
        self.assertEqual(0 in cache._blocks, True)
        self.assertEqual(1 in cache._blocks, True)
        self.assertEqual(2 in cache._blocks, True)
        self.assertEqual(cache._blocks[0]['active'], True)
        self.assertEqual(cache._blocks[0]['data'].size, 10)
        self.assertEqual(sum(cache._blocks[0]['data']), 20)
        self.assertEqual(cache._blocks[1]['active'], True)
        self.assertEqual(cache._blocks[1]['data'].size, 10)
        self.assertEqual(sum(cache._blocks[1]['data']), -16) # 2 + (9 * -2) fillvalue
        self.assertEqual(cache._blocks[2]['active'], True)
        self.assertEqual(cache._blocks[2]['data'].size, 10)
        self.assertEqual(sum(cache._blocks[2]['data']), -15) # 3 + (9 * -2) fillvalue

        # Now flush the blocks
        ds = _TestMockDataset()
        cache.flush(ds)

        # Verify the flushed slices are expected (2 full blocks and the last partial)
        self.assertEqual(ds.keys, [slice(0,10,None), slice(10,20,None), slice(20,26,None)])
        self.assertEqual(ds.values[0][0], 2)
        self.assertEqual(ds.values[0][1], 2)
        self.assertEqual(ds.values[0][2], 2)
        self.assertEqual(ds.values[0][3], 2)
        self.assertEqual(ds.values[0][4], 2)
        self.assertEqual(ds.values[0][5], 2)
        self.assertEqual(ds.values[0][6], 2)
        self.assertEqual(ds.values[0][7], 2)
        self.assertEqual(ds.values[0][8], 2)
        self.assertEqual(ds.values[0][9], 2)
        self.assertEqual(ds.values[1][0], 2)
        self.assertEqual(ds.values[1][1], -2)
        self.assertEqual(ds.values[1][2], -2)
        self.assertEqual(ds.values[1][3], -2)
        self.assertEqual(ds.values[1][4], -2)
        self.assertEqual(ds.values[1][5], -2)
        self.assertEqual(ds.values[1][6], -2)
        self.assertEqual(ds.values[1][7], -2)
        self.assertEqual(ds.values[1][8], -2)
        self.assertEqual(ds.values[1][9], -2)

        # Now wait for 2 seconds
        time.sleep(2.0)

        # Now write another value into block[2] and then flush once again
        cache.add_value(3, 26)

        # Now flush again, block [0] and [1] should be deleted
        ds = _TestMockDataset()
        cache.flush(ds)

        self.assertEqual(len(cache._blocks), 1)
        self.assertEqual(0 in cache._blocks, False)
        self.assertEqual(1 in cache._blocks, False)
        self.assertEqual(2 in cache._blocks, True)
        self.assertEqual(cache._blocks[2]['active'], False)
        self.assertEqual(cache._blocks[2]['data'].size, 10)
        self.assertEqual(sum(cache._blocks[2]['data']), -10) # 6 + (8 * -2) fillvalue

        self.assertEqual(ds.keys, [slice(20,27,None)])
        self.assertEqual(ds.values[0][0], -2)
        self.assertEqual(ds.values[0][1], -2)
        self.assertEqual(ds.values[0][2], -2)
        self.assertEqual(ds.values[0][3], -2)
        self.assertEqual(ds.values[0][4], -2)
        self.assertEqual(ds.values[0][5], 3)
        self.assertEqual(ds.values[0][6], 3)

    def test_unlimited_cache_2D(self):
        """ verify that the cache functions as expected
        """

        # Create a cache
        cache = HDF5UnlimitedCache('test1', 'int32', -2, 10, 1, shape=(2,3))

        # Verify 1 block has been created, with a size of 10x2x3
        self.assertEqual(len(cache._blocks), 1)
        self.assertEqual(0 in cache._blocks, True)
        self.assertEqual(cache._blocks[0]['active'], True)
        self.assertEqual(cache._blocks[0]['data'].size, 60)

        # Add 11 items at increasing indexes and verify another
        # block is created
        value = [
            [1,2,3],
            [4,5,6]
        ]
        for offset in range(11):
            cache.add_value(value, offset)

        self.assertEqual(len(cache._blocks), 2)
        self.assertEqual(0 in cache._blocks, True)
        self.assertEqual(1 in cache._blocks, True)
        self.assertEqual(cache._blocks[0]['active'], True)
        self.assertEqual(cache._blocks[0]['data'].size, 60)
        self.assertEqual(numpy.sum(cache._blocks[0]['data']), 210)
        self.assertEqual(cache._blocks[1]['active'], True)
        self.assertEqual(cache._blocks[1]['data'].size, 60)

        # Add 1 item at index to create another block
        cache.add_value(value, 25)

        self.assertEqual(len(cache._blocks), 3)
        self.assertEqual(0 in cache._blocks, True)
        self.assertEqual(1 in cache._blocks, True)
        self.assertEqual(2 in cache._blocks, True)
        self.assertEqual(cache._blocks[0]['active'], True)
        self.assertEqual(cache._blocks[0]['data'].size, 60)
        self.assertEqual(numpy.sum(cache._blocks[0]['data']), 210)
        self.assertEqual(cache._blocks[1]['active'], True)
        self.assertEqual(cache._blocks[1]['data'].size, 60)
        self.assertEqual(numpy.sum(cache._blocks[1]['data']), -87) # 21 + (9 * -12) fillvalue
        self.assertEqual(cache._blocks[2]['active'], True)
        self.assertEqual(cache._blocks[2]['data'].size, 60)
        self.assertEqual(numpy.sum(cache._blocks[2]['data']), -87) # 21 + (9 * -12) fillvalue

        # Now flush the blocks
        ds = _TestMockDataset()
        cache.flush(ds)

        # Verify the flushed slices are expected (2 full blocks and the last partial)
        self.assertEqual(ds.keys, [slice(0,10,None), slice(10,20,None), slice(20,26,None)])
        self.assertEqual(ds.values[0][0][0][0], 1)
        self.assertEqual(ds.values[0][0][0][1], 2)
        self.assertEqual(ds.values[0][0][0][2], 3)
        self.assertEqual(ds.values[0][0][1][0], 4)
        self.assertEqual(ds.values[0][0][1][1], 5)
        self.assertEqual(ds.values[0][0][1][2], 6)
        self.assertEqual(ds.values[0][5][0][0], 1)
        self.assertEqual(ds.values[0][5][0][1], 2)
        self.assertEqual(ds.values[0][5][0][2], 3)
        self.assertEqual(ds.values[0][5][1][0], 4)
        self.assertEqual(ds.values[0][5][1][1], 5)
        self.assertEqual(ds.values[0][1][1][2], 6)
        self.assertEqual(ds.values[1][1][0][0], -2)
        self.assertEqual(ds.values[1][1][0][1], -2)
        self.assertEqual(ds.values[1][1][0][2], -2)
        self.assertEqual(ds.values[1][1][1][0], -2)
        self.assertEqual(ds.values[1][1][1][1], -2)
        self.assertEqual(ds.values[1][1][1][2], -2)

        # Now wait for 2 seconds
        time.sleep(2.0)

        # Now write another value into block[2] and then flush once again
        cache.add_value(value, 26)

        # Now flush again, block [0] and [1] should be deleted
        ds = _TestMockDataset()
        cache.flush(ds)

        self.assertEqual(len(cache._blocks), 1)
        self.assertEqual(0 in cache._blocks, False)
        self.assertEqual(1 in cache._blocks, False)
        self.assertEqual(2 in cache._blocks, True)
        self.assertEqual(cache._blocks[2]['active'], False)
        self.assertEqual(cache._blocks[2]['data'].size, 60)
        self.assertEqual(numpy.sum(cache._blocks[2]['data']), -54) # 42 + (8 * -12) fillvalue

        self.assertEqual(ds.keys, [slice(20,27,None)])
        self.assertEqual(ds.values[0][0][0][0], -2)
        self.assertEqual(ds.values[0][0][0][1], -2)
        self.assertEqual(ds.values[0][0][0][2], -2)
        self.assertEqual(ds.values[0][0][1][0], -2)
        self.assertEqual(ds.values[0][0][1][1], -2)
        self.assertEqual(ds.values[0][0][1][2], -2)

        self.assertEqual(ds.values[0][5][0][0], 1)
        self.assertEqual(ds.values[0][5][0][1], 2)
        self.assertEqual(ds.values[0][5][0][2], 3)
        self.assertEqual(ds.values[0][5][1][0], 4)
        self.assertEqual(ds.values[0][5][1][1], 5)
        self.assertEqual(ds.values[0][5][1][2], 6)

        self.assertEqual(ds.values[0][6][0][0], 1)
        self.assertEqual(ds.values[0][6][0][1], 2)
        self.assertEqual(ds.values[0][6][0][2], 3)
        self.assertEqual(ds.values[0][6][1][0], 4)
        self.assertEqual(ds.values[0][6][1][1], 5)
        self.assertEqual(ds.values[0][6][1][2], 6)
