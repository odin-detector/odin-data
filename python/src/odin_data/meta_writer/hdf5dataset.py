import logging
from time import time

import h5py as h5
import numpy as np


class HDF5CacheBlock(object):
    def __init__(self, shape, fillvalue, dtype):
        self.has_new_data = True
        self.flush_time = time()
        self.data = np.full(shape, fillvalue, dtype)


class HDF5UnlimitedCache(object):
    """An object to represent a cache used by an HDF5Dataset"""

    def __init__(self, name, dtype, fillvalue, block_size, data_shape, block_timeout):
        """
        Args:
            name(str): Name of underlying dataset
            dtype(str) Datatype of cache blocks
            fillvalue(value matching dtype): Fill value of cache blocks
            block_size(int): Number of data elements within a cache block
            data_shape(tuple(int)): Shape of individual data elements in a cache block
            block_timeout(int): Timeout in seconds for a block
                If no new values are added within the timeout the block will be marked
                for deletion.

        """
        self._name = name
        self._dtype = dtype
        self._fillvalue = fillvalue
        self._block_size = block_size
        self._blocks = {}
        self._highest_index = -1
        self._block_shape = (block_size,) + data_shape
        self._block_timeout = block_timeout

        self._blocks[0] = HDF5CacheBlock(
            self._block_shape, self._fillvalue, self._dtype
        )

        self._logger = logging.getLogger("HDF5UnlimitedCache")
        self._logger.debug("Created cache for {}".format(self._name))

    def add_value(self, value, offset=None):
        if offset is None:
            offset = self._highest_index + 1
        if offset > self._highest_index:
            self._highest_index = offset
        block_index, value_index = divmod(offset, self._block_size)
        if block_index not in self._blocks:
            self._logger.debug("New cache block required")
            self._blocks[block_index] = HDF5CacheBlock(
                self._block_shape, self._fillvalue, self._dtype
            )
        self._blocks[block_index].has_new_data = True
        self._blocks[block_index].data[value_index] = value

    def purge_blocks(self):
        # Store to mark blocks for deletion
        purged_blocks = []
        # Iterate over a list copy as we may delete blocks
        for block_index, block in list(self._blocks.items()):
            if not block.has_new_data:
                # If the block has no new data check if it has become stale and delete
                time_since_flush = time() - block.flush_time
                if time_since_flush > self._block_timeout:
                    purged_blocks.append(block_index)
                    del self._blocks[block_index]
        self._logger.debug(
            "[{}] Purged {} stale blocks: {}".format(
                self._name, len(purged_blocks), purged_blocks
            )
        )

    def flush(self, h5py_dataset):
        # Loop over the blocks and write each one to the dataset
        self._logger.debug(
            "[{}] Highest recorded index: {}".format(self._name, self._highest_index)
        )
        self._logger.debug(
            "[{}] Flushing: total number of blocks to check: {}".format(
                self._name, len(self._blocks)
            )
        )
        # Record the number of blocks that have new data
        active_blocks = []
        for block_index, block in self._blocks.items():
            if block.has_new_data:
                # This block has changed since it was last written out
                active_blocks.append(block_index)
                self.write_block_to_dataset(block_index, block, h5py_dataset)
                block.has_new_data = False
                block.flush_time = time()  # Seconds since epoch
        self._logger.debug(
            "[{}] Flushing complete - flushed {} blocks: {}".format(
                self._name, len(active_blocks), active_blocks
            )
        )

    def write_block_to_dataset(self, block_index, block, h5py_dataset):
        block_start = block_index * self._block_size
        block_stop = block_start + self._block_size
        current_block_size = self._block_size
        if block_stop > self._highest_index:
            block_stop = self._highest_index + 1
            current_block_size = block_stop - block_start
        self._logger.debug(
            "[{}] Writing block {} [{}:{}] = {}".format(
                self._name,
                block_index,
                block_start,
                block_stop,
                np.array2string(block.data, threshold=10),
            )
        )
        if block_stop > h5py_dataset.len():
            h5py_dataset.resize(block_stop, axis=0)
        h5py_dataset[block_start:block_stop] = block.data[0:current_block_size]


class HDF5Dataset(object):
    """A wrapper of h5py.Dataset with a cache to reduce I/O"""

    def __init__(
        self,
        name,
        dtype,
        fillvalue,
        rank=1,
        shape=None,
        maxshape=None,
        chunks=None,
        cache=True,
        block_size=1000000,
        block_timeout=600,
    ):
        """
        Args:
            name(str): Name to pass to h5py.Dataset (and for log messages)
            dtype(str) Datatype to pass to h5py.Dataset
            fillvalue(value matching dtype): Fill value for h5py.Dataset
            rank(int): The rank (number of dimensions) of the dataset
            shape(tuple(int)): Initial shape to pass to h5py.Dataset
            maxshape(tuple(int)): Maximum shape to pass to h5py.Dataset. Provide this to set
                the maximum dataset size.
            cache(bool): Whether to store a local cache of values
                         or write directly to file
            block_size(int): See HDF5UnlimitedCache
            block_timeout(int): See HDF5UnlimitedCache

        """
        self.name = name
        self.dtype = dtype
        self.fillvalue = fillvalue
        self.shape = shape if shape is not None else (0,) * rank
        self.chunks = chunks
        self.maxshape = (
            maxshape
            if maxshape is not None
            else shape
            if shape is not None
            else (None,) * rank
        )
        self._cache = None

        data_shape = self.shape[1:]  # The shape of each data element in dataset
        assert not (cache and 0 in data_shape), (
            "Must define full initial shape to use cache - only first axis can be 0. "
            "e.g (0, 256) for a dataset that will be (N, 256) when complete"
        )

        if cache:
            self._cache = HDF5UnlimitedCache(
                name=self.name,
                dtype=self.dtype,
                fillvalue=self.fillvalue,
                block_size=block_size,
                data_shape=data_shape,
                block_timeout=block_timeout,
            )

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
                self._h5py_dataset.resize(self._h5py_dataset.len() + 1, axis=0)
                self._h5py_dataset[-1] = value
            else:
                if offset + 1 > self._h5py_dataset.len():
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

        if self.shape != data.shape:
            try:
                self._h5py_dataset.resize(data.shape)
            except Exception as exception:
                self._logger.error(
                    "%s | Data shape %s does not match dataset shape %s"
                    " and resize failed:\n%s",
                    self.name,
                    data.shape,
                    self.shape,
                    exception,
                )
                return

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
                self._logger.debug("%s | Writing cache to dataset", self.name)
                self._cache.flush(self._h5py_dataset)
                self._cache.purge_blocks()
            self._h5py_dataset.flush()


class Int32HDF5Dataset(HDF5Dataset):
    """Int32 HDF5Dataset"""

    def __init__(
        self,
        name,
        fillvalue=-1,
        shape=None,
        maxshape=None,
        chunks=None,
        cache=True,
        block_size=1000000,
        block_timeout=600,
        **kwargs,
    ):
        super(Int32HDF5Dataset, self).__init__(
            name,
            dtype="int32",
            fillvalue=fillvalue,
            shape=shape,
            maxshape=maxshape,
            chunks=chunks,
            cache=cache,
            block_size=block_size,
            block_timeout=block_timeout,
            **kwargs,
        )


class Int64HDF5Dataset(HDF5Dataset):
    """Int64 HDF5Dataset"""

    def __init__(
        self,
        name,
        fillvalue=-1,
        shape=None,
        maxshape=None,
        cache=True,
        block_size=1000000,
        block_timeout=600,
        **kwargs,
    ):
        super(Int64HDF5Dataset, self).__init__(
            name,
            dtype="int64",
            fillvalue=fillvalue,
            shape=shape,
            maxshape=maxshape,
            cache=cache,
            block_size=block_size,
            block_timeout=block_timeout,
            **kwargs,
        )


class Float32HDF5Dataset(HDF5Dataset):
    """Float32 HDF5Dataset"""

    def __init__(
        self,
        name,
        shape=None,
        maxshape=None,
        cache=True,
        block_size=1000000,
        block_timeout=600,
        **kwargs,
    ):
        super(Float32HDF5Dataset, self).__init__(
            name,
            dtype="float32",
            fillvalue=-1,
            shape=shape,
            maxshape=maxshape,
            cache=cache,
            block_size=block_size,
            block_timeout=block_timeout,
            **kwargs,
        )


class Float64HDF5Dataset(HDF5Dataset):
    """Float64 HDF5Dataset"""

    def __init__(
        self,
        name,
        shape=None,
        maxshape=None,
        cache=True,
        block_size=1000000,
        block_timeout=600,
        **kwargs,
    ):
        super(Float64HDF5Dataset, self).__init__(
            name,
            dtype="float64",
            fillvalue=-1,
            shape=shape,
            maxshape=maxshape,
            cache=cache,
            block_size=block_size,
            block_timeout=block_timeout,
            **kwargs,
        )


class StringHDF5Dataset(HDF5Dataset):
    """String HDF5Dataset"""

    def __init__(
        self,
        name,
        length=None,
        encoding="utf-8",
        shape=None,
        maxshape=None,
        cache=True,
        block_size=1000000,
        block_timeout=600,
        **kwargs,
    ):
        """
        Args:
            length(int): Length of string - None to store as variable-length objects
            encoding(str): Encoding of string - either `"utf-8"` or `"ascii"`

        """
        self.dtype = h5.string_dtype(encoding=encoding, length=length)
        super(StringHDF5Dataset, self).__init__(
            name,
            dtype=self.dtype,
            fillvalue="",
            shape=shape,
            maxshape=maxshape,
            cache=cache,
            block_size=block_size,
            block_timeout=block_timeout,
            **kwargs,
        )
        if length and not maxshape:
            self.maxshape = None

    def prepare_data(self, data):
        """Prepare data ready to write to hdf5 dataset

        Ensure data array contains the correct string dtype by converting first to bytes
        and then to the specific string type. This initial conversion is required if
        self.dtype is a variable-length string object data is not a string type because,
        e.g., np.asarray(ndarray[int], object) leaves the data as int, but int cannot be
        implicitly converted to an h5py dataset of dtype 'O' on write.

        Args:
            data(np.ndarray): Data to format

        Returns:
            data(np.ndarray): Formatted data

        """
        data = super(StringHDF5Dataset, self).prepare_data(data)

        if self.dtype.kind == "O" and data.dtype.kind != "S":
            # Ensure data is bytes before converting to variable-length object
            data = np.asarray(data, dtype=bytes)

        # Convert to specific string dtype - either variable-length object or
        # fixed-length string. No copy is performed if the dtype already matches.
        data = np.asarray(data, dtype=self.dtype)

        return data
