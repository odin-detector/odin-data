"""Implementation of odin_data Meta Writer

This module is passed meta data messages for a single acquisition which it writes to disk.
Will need to be subclassed by detector specific implementation.

Matt Taylor, Diamond Light Source
"""
import h5py
import os
import time


class MetaWriter(object):
    """Meta Writer class.

    This class handles meta data messages and writes meta data to disk
    """

    FILE_SUFFIX = '_meta.h5'

    def __init__(self, logger, directory, acquisition_id):
        """Initalise the MetaWriter object.

        :param logger: Logger to use
        :param directory: Directory to create the meta file in
        :param acquisition_id: Acquisition ID of this acquisition
        """
        self._logger = logger
        self._acquisition_id = acquisition_id
        self._num_frames_to_write = -1
        self.number_processes_running = 0
        self.directory = directory
        self.finished = False
        self.write_count = 0
        self.write_timeout_count = 0
        self.flush_frequency = 100
        self.flush_timeout = None
        self._last_flushed = time.time()
        self.file_created = False
        self._hdf5_file = None
        self.full_file_name = None

        self._data_set_definition = {}
        self._hdf5_datasets = {}
        self._data_set_arrays = {}

    def start_new_acquisition(self):
        """Override to perform actions needed when the acquisition is started."""
        self._logger.warn("Parent start_new_acquisition has been called. Should be overridden")
        return

    def process_message(self, message, userheader, receiver):
        """Override to perform actions needed when a message is received.

        :param message: The message received
        :param userheader: The user header of the message parsed from the original meta message
        :param receiver: The ZeroMQ socket that the message was received on
        """
        self._logger.warn("Parent process_message has been called. Should be overridden")
        self._logger.debug(message)

    def create_file(self):
        """Create the meta file to write data into."""
        meta_file_name = self._acquisition_id + self.FILE_SUFFIX
        self.full_file_name = os.path.join(self.directory, meta_file_name)
        self._logger.info("Writing meta data to: %s" % self.full_file_name)
        self._hdf5_file = h5py.File(self.full_file_name, "w", libver='latest')
        self.create_datasets()
        self.file_created = True

    def create_datasets(self):
        """Create datasets for each definition."""
        for dset_name in self._data_set_definition:
            dset = self._data_set_definition[dset_name]
            self._data_set_arrays[dset_name] = []
            self._hdf5_datasets[dset_name] = self._hdf5_file.create_dataset(dset_name,
                                                                            dset['shape'],
                                                                            maxshape=dset['maxshape'],
                                                                            dtype=dset['dtype'],
                                                                            fillvalue=dset['fillvalue'])

    def create_dataset_with_data(self, dset_name, data, shape=None):
        """Create a dataset using pre existing data.

        :param dset_name: The dataset name
        :param data: The data to write
        :param shape: Shape of the data
        """
        if shape is not None:
            self._hdf5_datasets[dset_name] = self._hdf5_file.create_dataset(dset_name, shape, data=data)
        else:
            self._hdf5_datasets[dset_name] = self._hdf5_file.create_dataset(dset_name, data=data)
        self._hdf5_datasets[dset_name].flush()

    def add_dataset_definition(self, dset_name, shape, maxshape, dtype, fillvalue):
        """Add a dataset definition to the list.

        :param dset_name: The dataset name
        :param shape: Shape of the data
        :param maxshape: The maximum shape of the data
        :param dtype: The type of data
        :param fillvalue: The fill value to use
        """
        self._data_set_definition[dset_name] = {
            'shape': shape,
            'maxshape': maxshape,
            'dtype': dtype,
            'fillvalue': fillvalue
        }

    def add_dataset_value(self, dset_name, value):
        """Append a value to the named dataset array.

        :param dset_name: The dataset name
        :param value: The value to append
        """
        self._data_set_arrays[dset_name].append(value)
        # Reset timeout count to 0
        self.write_timeout_count = 0

    def write_datasets(self):
        """Override to perform actions needed to write the data to disk."""
        self._logger.warn("Parent write_datasets has been called. Should be overridden")
        for dset_name in self._data_set_definition:
            self._logger.warn("Length of [%s] dataset: %d", dset_name, len(self._data_set_arrays[dset_name]))
            self._hdf5_datasets[dset_name].resize((len(self._data_set_arrays[dset_name]),))
            self._hdf5_datasets[dset_name][0:len(self._data_set_arrays[dset_name])] = self._data_set_arrays[dset_name]

    def close_file(self):
        """Override to perform actions needed to close the file."""
        self.write_datasets()

        if self._hdf5_file is not None:
            self._logger.info('Closing file ' + self.full_file_name)
            self._hdf5_file.close()
            self._hdf5_file = None

        self.finished = True

    def stop(self):
        """Override to perform actions needed to stop writing."""
        self.close_file()

