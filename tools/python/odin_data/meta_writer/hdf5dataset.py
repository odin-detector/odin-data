import logging
from time import time

import numpy as np


class HDF5UnlimitedCache(object):
    """ An object to represent a cache used by an HDF5Dataset """
    def __init__(self, name, dtype, fillvalue, block_size, stale_time, shape=None):
        self._name = name
        self._dtype = dtype
        self._fillvalue = fillvalue
        self._block_size = block_size
        self._blocks = {}
        self._highest_index = 0
        if shape is None:
            self._shape = block_size
        else:
            self._shape = (block_size,) + shape
        self._logger = logging.getLogger("HDF5UnlimitedCache")
        self._logger.debug("Creating unlimited HDF5UnlimitedCache for {}".format(self._name))
        self._blocks[0] = self.new_array()
        self._stale_time = stale_time


    def new_array(self):
        self._logger.debug("[{}] Appending new numpy array of {} to cache in unlimited mode".format(self._name, self._block_size))
        data = {
            'active': True,
            'flush_time': time(),
            'data': np.full(self._shape, self._fillvalue, self._dtype)
        }
        return data

    def add_value(self, value, offset):
        if offset > self._highest_index:
            self._highest_index = offset
        block_index, value_index = divmod(offset, self._block_size)
        if block_index not in self._blocks:
            self._logger.debug("New cache block required")
            self._blocks[block_index] = self.new_array()
        self._blocks[block_index]['active'] = True
        self._blocks[block_index]['data'][value_index] = value

    def flush(self, h5py_dataset):
        # Loop over the blocks and write each one to the dataset
        self._logger.debug("[{}] Highest recorded index: {}".format(self._name, self._highest_index))
        index = 0
        self._logger.debug("[{}] Flushing: total number of blocks to check: {}".format(self._name, len(self._blocks)))
        # Store to mark blocks for deletion
        delete_blocks = []
        # Record the number of active blocks
        active_blocks = []
        for block in self._blocks:
            if self._blocks[block]['active']:
                active_blocks.append(block)
                index = block * self._block_size
                upper_index = index + self._block_size
                current_block_size = self._block_size
                if upper_index > self._highest_index:
                    upper_index = self._highest_index + 1
                    current_block_size = upper_index - index
                self._logger.debug("[{}] Flushing block {} to index {}:{}".format(self._name, block, index, upper_index))
                self._logger.debug("[{}] Current block size:{}".format(self._name, current_block_size))
                current_block = self._blocks[block]['data']
                self._logger.debug("[{}] Block slice to write:{}".format(self._name, current_block[0:current_block_size]))
                if upper_index > h5py_dataset.len():
                    self._logger.debug("[{}] Increasing dataset size from {} to {}".format(self._name, h5py_dataset.len(), upper_index))
                    h5py_dataset.resize(upper_index, axis=0)
                h5py_dataset[index:upper_index] = current_block[0:current_block_size]
                self._blocks[block]['active'] = False
                self._blocks[block]['flush_time'] = time() # Seconds since epoch
            else:
                # If the block is inactive check if it has become stale and mark for deletion
                stale_time = time() - self._blocks[block]['flush_time']
                if stale_time > self._stale_time:
                    delete_blocks.append(block)
        for block in delete_blocks:
            stale_time = time() - self._blocks[block]['flush_time']
            self._logger.debug("[{}] Block {} marked as stale after {} seconds, deleting...".format(self._name, block, stale_time))
            del self._blocks[block]
        self._logger.debug("[{}] Flushing complete - flushed {} active blocks: {}".format(self._name, len(active_blocks), active_blocks))


class HDF5Dataset(object):
    """A wrapper of h5py.Dataset with a cache to reduce I/O"""

    def __init__(self, name, dtype, fillvalue, rank=1, shape=None, cache=True, block_size=1000000, block_timeout=600):
        """
        Args:
            name(str): Name to pass to h5py.Dataset (and for log messages)
            dtype(str) Datatype to pass to h5py.Dataset
            fillvalue(value matching dtype): Fill value for h5py.Dataset
            rank(int): The rank (number of dimensions) of the dataset
            shape(tuple(int)): Shape to pass to h5py.Dataset
            cache(bool): Whether to store a local cache of values
                         or write directly to file

        """
        self.name = name
        self.dtype = dtype
        self.fillvalue = fillvalue
        self.shape = shape if shape is not None else (0,)*rank
        self.maxshape = shape if shape is not None else (None,)*rank
        self.block_size = block_size
        self.block_timeout = block_timeout
        self._cache = None

        if cache:
            self._cache = HDF5UnlimitedCache(self.name, self.dtype, self.fillvalue, self.block_size, self.block_timeout, shape=shape)

        self._h5py_dataset = None  # h5py.Dataset
        self._is_written = False

        self._logger = logging.getLogger("HDF5Dataset")

    def initialise(self, dataset_handle, dataset_size):
        """Initialise _h5py_dataset handle and cache

        Args:
            dataset_handle(h5py.Dataset): Actual h5py Dataset object
            dataset_size(int): Length of datasets

        """
        self._h5py_dataset = dataset_handle

    def add_value(self, value, offset=None):
        """Add a value at the given offset

        If we are caching parameters, add the value to the cache, otherwise
        extend the dataset and write it directly.

        Args:
            value(object matching dtype): Value to add to dataset
            offset(int): Offset to insert value at in dataset

        """
        if self._cache is None:
            if offset is None:
                self._h5py_dataset.resize(self._h5py_dataset.shape[0] + 1, axis=0)
                self._h5py_dataset[-1] = value
            else:
                if offset + 1 > self._h5py_dataset.shape[0]:
                    self._h5py_dataset.resize(offset + 1, axis=0)
                self._h5py_dataset[offset] = value
            return

        else:
            self._cache.add_value(value, offset)

    def write(self, data):
        """Write the entire dataset with the given data

        Args:
            data(np.ndarray): Data to set HDF5 dataset with

        """
        if self._cache is not None:
            self._logger.error(
                "%s | Cannot write entire dataset when cache is enabled", self.name
            )
            return

        if self._is_written:
            self._logger.debug("%s | Already written", self.name)
            return

        data = self.prepare_data(data)

        self._h5py_dataset.resize(data.shape)
        self._h5py_dataset[...] = data
        self._is_written = True

    def prepare_data(self, data):
        """Prepare data ready to write to hdf5 dataset

        Ensure data is a 1D (or more) numpy array

        Args:
            data(): Input data

        Returns:
            data(np.ndarray): Output data

        """
        data = np.asarray(data)
        if data.shape == ():
            data = data.reshape((1,))

        return data

    def flush(self):
        """Write cached values to file (if cache enabled) and call flush"""
        if self._h5py_dataset is not None:
            if self._cache is not None:
                self._logger.debug(
                    "%s | Writing cache to dataset",
                    self.name)
                self._cache.flush(self._h5py_dataset)
            self._h5py_dataset.flush()


class Int32HDF5Dataset(HDF5Dataset):
    """Int32 HDF5Dataset"""

    def __init__(self, name, fillvalue=-1, shape=None, cache=True, block_size=1000000, block_timeout=600):
        super(Int32HDF5Dataset, self).__init__(
            name, dtype="int32", fillvalue=fillvalue, shape=shape, cache=cache, block_size=block_size, block_timeout=block_timeout
        )


class Int64HDF5Dataset(HDF5Dataset):
    """Int64 HDF5Dataset"""

    def __init__(self, name, fillvalue=-1, shape=None, cache=True, block_size=1000000, block_timeout=600, **kwargs):
        super(Int64HDF5Dataset, self).__init__(
            name, dtype="int64", fillvalue=fillvalue, shape=shape, cache=cache, block_size=block_size, block_timeout=block_timeout, **kwargs
        )


class Float64HDF5Dataset(HDF5Dataset):
    """Int64 HDF5Dataset"""

    def __init__(self, name, shape=None, cache=True, block_size=1000000, block_timeout=600, **kwargs):
        super(Float64HDF5Dataset, self).__init__(
            name, dtype="float64", fillvalue=-1, shape=shape, cache=cache, block_size=block_size, block_timeout=block_timeout, **kwargs
        )


class StringHDF5Dataset(HDF5Dataset):
    """String HDF5Dataset"""

    def __init__(self, name, length, shape=None, cache=True, block_size=1000000, block_timeout=600):
        """
        Args:
            length(int): Maximum length of the string elements

        """
        super(StringHDF5Dataset, self).__init__(
            name, dtype="S{}".format(length), fillvalue="", shape=shape, cache=cache, block_size=block_size, block_timeout=block_timeout
        )

    def prepare_data(self, data):
        """Prepare data ready to write to hdf5 dataset

        Convert data to raw strings to write to hdf5 dataset

        Args:
            data(np.ndarray): Data to format

        Returns:
            data(np.ndarray): Formatted data

        """
        data = super(StringHDF5Dataset, self).prepare_data(data)

        # Make sure data array contains str
        data = np.array(data, dtype=str)

        return data
