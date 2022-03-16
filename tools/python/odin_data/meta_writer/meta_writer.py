"""Implementation of odin_data Meta Writer

This module is passed meta data messages for a single acquisition which it writes to disk.
Will need to be subclassed by detector specific implementation.

Matt Taylor, Diamond Light Source
"""
import os
from time import time
import logging

import h5py

from odin_data import _version as versioneer
from odin_data.util import construct_version_dict
from .hdf5dataset import HDF5Dataset, Int64HDF5Dataset

# Data message parameters
FRAME = "frame"
OFFSET = "offset"
RANK = "rank"
ENDPOINT = "_endpoint"
CREATE_DURATION = "create_duration"
WRITE_DURATION = "write_duration"
FLUSH_DURATION = "flush_duration"
CLOSE_DURATION = "close_duration"
MESSAGE_TYPE_ID = "parameter"

# Configuration items
DIRECTORY = "directory"
FILE_PREFIX = "file_prefix"
FLUSH_FRAME_FREQUENCY = "flush_frame_frequency"
FLUSH_TIMEOUT = "flush_timeout"


class MetaWriterConfig(object):

    def __init__(self, sensor_shape):
        """
        Args:
            sensor_shape(tuple): Detector sensor size (y, x)

        """
        self.sensor_shape = sensor_shape


def require_open_hdf5_file(func):
    """A decorator to verify the HDF5 file is open before calling the wrapped method

    If the HDF5 file is currently open for writing, call the method, else log the reason
    the file is not open

    NOTE: This should only be used on MetaWriter methods (that take self as the first
    argument)

    """

    def wrapper(*args, **kwargs):
        writer = args[0]  # Extract class instance (self) from args

        if writer.file_open:
            # It is safe to call the wrapped method
            return func(*args, **kwargs)

        # It is not safe to call the wrapped method - log the reason why
        if writer.finished:
            reason = "Already finished writing"
        else:
            reason = "Have not received startacquisition yet"
        writer._logger.error(
            "%s | Cannot call %s - File not open - %s",
            writer._name,
            func.__name__,
            reason,
        )

    return wrapper


class MetaWriter(object):
    """This class handles meta data messages and writes parameters to disk"""

    FILE_SUFFIX = "_meta.h5"
    CONFIGURE_PARAMETERS = [
        DIRECTORY,
        FILE_PREFIX,
        FLUSH_FRAME_FREQUENCY,
        FLUSH_TIMEOUT,
    ]
    # Detector-specific parameters received on per-frame meta message
    DETECTOR_WRITE_FRAME_PARAMETERS = []

    def __init__(self, name, directory, endpoints, config):
        """
        Args:
            name(str): Unique name to construct file path and to include in
                       log messages
            directory(str): Directory to create the meta file in
            endpoints(list): Endpoints parent MetaListener will receive data on
            config(MetaWriterConfig): Configuration options

        """
        self._logger = logging.getLogger(self.__class__.__name__)

        # Config
        self.directory = directory
        self.file_prefix = None
        self.flush_frame_frequency = 100
        self.flush_timeout = 1

        # Status
        self.full_file_path = None
        self.write_count = 0
        self.finished = False
        self.write_timeout_count = 0

        # Internal parameters
        self._name = name
        self._processes_running = [False] * len(endpoints)
        self._endpoints = endpoints
        self._config = config
        self._last_flushed = time()  # Seconds since epoch
        self._frames_since_flush = 0
        self._hdf5_file = None
        self._datasets = dict(
            (dataset.name, dataset)
            for dataset in self._define_datasets() + self._define_detector_datasets()
        )
        # Child class parameters
        self._frame_data_map = dict()  # Map of frame number to detector data
        self._frame_offset_map = dict()  # Map of frame number to offset in dataset
        self._writers_finished = False
        self._detector_finished = True  # See stop_when_detector_finished

    @staticmethod
    def _define_datasets():
        return [
            Int64HDF5Dataset(FRAME),
            Int64HDF5Dataset(OFFSET),
            Int64HDF5Dataset(CREATE_DURATION, cache=False),
            Int64HDF5Dataset(WRITE_DURATION),
            Int64HDF5Dataset(FLUSH_DURATION),
            Int64HDF5Dataset(CLOSE_DURATION, cache=False),
        ]

    def _define_detector_datasets(self):
        return []

    @property
    def file_open(self):
        return self._hdf5_file is not None

    @property
    def active_process_count(self):
        return self._processes_running.count(True)

    def _generate_full_file_path(self):
        prefix = self.file_prefix if self.file_prefix is not None else self._name
        self.full_file_path = os.path.join(
            self.directory, "{}{}".format(prefix, self.FILE_SUFFIX)
        )
        return self.full_file_path

    def _create_file(self, file_path, dataset_size):
        self._logger.debug(
            "%s | Opening file %s - Expecting %d frames",
            self._name,
            file_path,
            dataset_size,
        )

        try:
            self._hdf5_file = h5py.File(file_path, "w", libver="latest")
        except IOError as error:
            self._logger.error(
                "%s | Failed to create file:\n%s: %s",
                self._name,
                error.__class__.__name__,
                error.message,
            )
            return

        self._create_datasets(dataset_size)
        # Datasets created after this point will not be SWMR-readable
        self._hdf5_file.swmr_mode = True

    @require_open_hdf5_file
    def _close_file(self):
        self._logger.info("%s | Closing file", self._name)

        self._flush_datasets()
        self._hdf5_file.close()
        self._hdf5_file = None

    @require_open_hdf5_file
    def _create_datasets(self, dataset_size):
        """Add predefined datasets to HDF5 file and store handles

        Args:
            datasets(list(HDF5Dataset)): The datasets to add to the file

        """
        self._logger.debug("%s | Creating datasets", self._name)

        for dataset in self._datasets.values():
            chunks = dataset.maxshape
            if isinstance(chunks, int):
                chunks = (chunks,)
            if None in chunks:
                chunks = None
            dataset_handle = self._hdf5_file.create_dataset(
                name=dataset.name,
                shape=dataset.shape,
                maxshape=dataset.maxshape,
                chunks=chunks,
                dtype=dataset.dtype,
                fillvalue=dataset.fillvalue,
            )
            dataset.initialise(dataset_handle, dataset_size)

    @require_open_hdf5_file
    def _add_dataset(self, dataset_name, data, dataset_size=None):
        """Add a new dataset with the given data

        Args:
            dataset_name(str): Name of dataset
            data(np.ndarray): Data to initialise HDF5 dataset with
            dataset_size(int): Dataset size - required if more data will be added

        """
        self._logger.debug("%s | Adding dataset %s", self._name, dataset_name)

        if dataset_name in self._datasets:
            self._logger.debug(
                "%s | Dataset %s already created", self._name, dataset_name
            )
            return

        self._logger.debug(
            "%s | Creating dataset %s with data:\n%s", self._name, dataset_name, data
        )
        dataset = HDF5Dataset(dataset_name, dtype=None, fillvalue=None, cache=False)
        dataset_handle = self._hdf5_file.create_dataset(name=dataset_name, data=data)
        dataset.initialise(dataset_handle, dataset_size)

        self._datasets[dataset_name] = dataset

    @require_open_hdf5_file
    def _add_value(self, dataset_name, value, offset=0):
        """Append a value to the named dataset

        Args:
            dataset_name(str): Name of dataset
            value(): The value to append
            index(int): The offset to add the value to

        """
        self._logger.debug("%s | Adding value to %s", self._name, dataset_name)

        if dataset_name not in self._datasets:
            self._logger.error("%s | No such dataset %s", self._name, dataset_name)
            return

        self._datasets[dataset_name].add_value(value, offset)

    @require_open_hdf5_file
    def _add_values(self, expected_parameters, data, offset):
        """Take values of parameters from data and write to datasets at offset

        Args:
            expected_parameters(list(str)): Parameters to write
            data(dict): Set of parameter values
            offset(int): Offset to write parameters to in datasets

        """
        self._logger.debug("%s | Adding values to datasets", self._name)

        for parameter in expected_parameters:
            if parameter not in data:
                self._logger.error(
                    "%s | Expected parameter %s not found in %s",
                    self._name,
                    parameter,
                    data,
                )
                continue

            self._add_value(parameter, data[parameter], offset)

    @require_open_hdf5_file
    def _write_dataset(self, dataset_name, data):
        """Write an entire dataset with the given data

        Args:
            dataset_name(str): Name of dataset
            data(np.ndarray): Data to set HDF5 dataset with

        """
        self._logger.debug("%s | Writing entire dataset %s", self._name, dataset_name)

        if dataset_name not in self._datasets:
            self._logger.error("%s | No such dataset %s", self._name, dataset_name)
            return

        self._datasets[dataset_name].write(data)

    @require_open_hdf5_file
    def _write_datasets(self, expected_parameters, data):
        """Take values of parameters from data and write datasets

        Args:
            expected_parameters(list(str)): Parameters to write
            data(dict): Set of parameter values

        """
        self._logger.debug("%s | Writing datasets", self._name)

        for parameter in expected_parameters:
            if parameter not in data:
                self._logger.error(
                    "%s | Expected parameter %s not found in %s",
                    self._name,
                    parameter,
                    data,
                )
                continue

            self._write_dataset(parameter, data[parameter])

    @require_open_hdf5_file
    def _flush_datasets(self):
        self._logger.debug("%s | Flushing datasets", self._name)

        for dataset in self._datasets.values():
            dataset.flush()

    def stop_when_detector_finished(self):
        """Register that it is OK to stop when all detector-specific logic is complete

        By default, detector_finished is set to True initially so that this check always
        passes. Child classes that need to do their own checks can set this to False in
        __init__ and call stop_when_writers_finished when ready to stop.

        """
        self._writers_finished = True
        if self._detector_finished:
            self._logger.debug("%s | Detector already finished", self._name)
            self.stop()
        else:
            self._logger.debug("%s | Detector not finished", self._name)

    def stop_when_writers_finished(self):
        """Register that it is OK to stop when all monitored writers have finished

        Child classes can call this when all detector-specific logic is complete.

        """
        self._detector_finished = True
        if self._writers_finished:
            self._logger.debug("%s | Writers already finished", self._name)
            self.stop()
        else:
            self._logger.debug("%s | Writers not finished", self._name)

    def stop(self):
        if self.file_open:
            self._close_file()
        self.finished = True
        self._logger.info("%s | Finished", self._name)

    def status(self):
        """Return current status parameters"""
        return dict(
            full_file_path=self.full_file_path,
            num_processors=self.active_process_count,
            written=self.write_count,
            writing=self.file_open and not self.finished,
        )

    def configure(self, configuration):
        """Configure the writer with a set of one or more parameters

        Args:
            configuration(dict): Configuration parameters

        Returns:
            error(None/str): None if successful else an error message

        """
        error = None
        for parameter, value in configuration.items():
            if parameter in self.CONFIGURE_PARAMETERS:
                self._logger.debug(
                    "%s | Setting %s to %s", self._name, parameter, value
                )
                setattr(self, parameter, value)
            else:
                error = "Invalid parameter {}".format(parameter)
                self._logger.error("%s | %s", self._name, error)

        return error

    def request_configuration(self):
        """Return the current configuration

        Returns:
            configuration(dict): Dictionary of current configuration parameters

        """
        configuration = dict(
            (parameter, getattr(self, parameter))
            for parameter in self.CONFIGURE_PARAMETERS
        )

        return configuration

    # Methods for handling various message types

    @property
    def message_handlers(self):
        """Dictionary of message type to handler method

        This should be overridden by child classes to add additional handlers

        Returns:
            dict: message type handler methods

        """
        message_handlers = {
            "startacquisition": self.handle_start_acquisition,
            "createfile": self.handle_create_file,
            "writeframe": self.handle_write_frame,
            "closefile": self.handle_close_file,
            "stopacquisition": self.handle_stop_acquisition,
        }

        message_handlers.update(self.detector_message_handlers)
        return message_handlers

    @property
    def detector_message_handlers(self):
        return {}

    def process_message(self, header, data):
        """Process a message from a data socket

        This is main entry point for handling any type of message and calling
        the appropriate method.

        Look up the appropriate message handler based on the message type and
        call it.

        Leading underscores on a handler function definition parameter mean it
        does not use the argument.

        This should be overridden by child classes to handle any additional messages.

        Args:
            header(str): The header message part
            data(str): The data message part (a json string or a data blob)

        """
        handler = self.message_handlers.get(header[MESSAGE_TYPE_ID], None)
        if handler is not None:
            handler(header["header"], data)
        else:
            self._logger.error(
                "%s | Unknown message type: %s", self._name, header[MESSAGE_TYPE_ID]
            )

    def handle_start_acquisition(self, header, _data):
        """Prepare the data file with the number of frames to write"""
        self._logger.debug("%s | Handling start acquisition message", self._name)

        if self._processes_running[self._endpoints.index(header[ENDPOINT])]:
            self._logger.error(
                "%s | Received additional startacquisition from process endpoint %s - ignoring",
                self._name,
                header[ENDPOINT],
            )
            return

        self._processes_running[self._endpoints.index(header[ENDPOINT])] = True

        self._logger.debug(
            "%s | Received startacquisition message from endpoint %s - %d processes running",
            self._name,
            header[ENDPOINT],
            self.active_process_count,
        )

        if not self.file_open:
            self._create_file(self._generate_full_file_path(), header["totalFrames"])

    def handle_create_file(self, _header, data):
        self._logger.debug("%s | Handling create file message", self._name)

        self._add_value(CREATE_DURATION, data[CREATE_DURATION])

    def handle_write_frame(self, _header, data):
        self._logger.debug("%s | Handling write frame message", self._name)

        # TODO: Handle getting more frames than expected because of rewinding?
        write_frame_parameters = [FRAME, OFFSET, WRITE_DURATION, FLUSH_DURATION]
        self._add_values(write_frame_parameters, data, data[OFFSET])

        # Here we keep track of whether we need to write to disk based on:
        #   - Time since last write
        #   - Number of write frame messages since last write

        # Reset timeout count to 0
        self.write_timeout_count = 0

        self.write_count += 1
        self._frames_since_flush += 1

        # Write detector meta data for this frame, now that we know the offset
        self.write_detector_frame_data(data[FRAME], data[OFFSET])

        flush_required = (
            time() - self._last_flushed >= self.flush_timeout
            or self._frames_since_flush >= self.flush_frame_frequency
        )

        if flush_required:
            self._flush_datasets()
            self._last_flushed = time()
            self._frames_since_flush = 0

    def write_detector_frame_data(self, frame, offset):
        """Write the frame data to at the given offset

        Args:
            frame(int): Frame to write
            offset(int): Offset in datasets to write the frame data to

        """
        if not self.DETECTOR_WRITE_FRAME_PARAMETERS:
            # No detector specific data to write
            return

        self._logger.debug("%s | Writing detector data for frame %d", self._name, frame)

        if frame not in self._frame_data_map:
            self._logger.warning(
                "%s | No detector meta data stored for frame %d", self._name, frame
            )
            self._frame_offset_map[frame] = offset
            return

        data = self._frame_data_map.pop(frame)
        self._add_values(self.DETECTOR_WRITE_FRAME_PARAMETERS, data, offset)

    def handle_close_file(self, _header, data):
        self._logger.debug("%s | Handling close file message", self._name)

        self._add_value(CLOSE_DURATION, data[CLOSE_DURATION])

    def handle_stop_acquisition(self, header, _data):
        """Register that a process has finished and stop if it is the last one"""
        if not self._processes_running[self._endpoints.index(header[ENDPOINT])]:
            self._logger.error(
                "%s | Received stopacquisition from process endpoint %s before start - ignoring",
                self._name,
                header[ENDPOINT],
            )
            return

        self._logger.debug(
            "%s | Received stopacquisition from endpoint %s", self._name, header[ENDPOINT]
        )
        self._processes_running[self._endpoints.index(header[ENDPOINT])] = False

        if not any(self._processes_running):
            self._logger.info("%s | Last processor stopped", self._name)
            self.stop_when_detector_finished()

    @staticmethod
    def get_version():
        return "odin-data", construct_version_dict(versioneer.get_versions()["version"])
