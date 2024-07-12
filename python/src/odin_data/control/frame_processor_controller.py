"""
Created on 30th November 2023

:author: Alan Greer
"""

import logging
import threading
import time

from odin.adapters.parameter_tree import ParameterTree


class FrameProcessorController(object):
    def __init__(self, name):
        logging.info("FrameProcessorController init called")
        self._acquisition_id = ""
        self._frames = 0
        self._name = name
        self._file_path = ""
        self._file_prefix = ""
        self._file_extension = "h5"
        self._fp_params = ParameterTree(
            {
                "module": (lambda: self._name, None, {}),
                "hdf": {
                    "acquisition_id": (
                        self._get("_acquisition_id"),
                        lambda v: self._set("_acquisition_id", v),
                        {},
                    ),
                    "frames": (
                        self._get("_frames"),
                        lambda v: self._set("_frames", v),
                        {},
                    ),
                    "file": {
                        "path": (
                            self._get("_file_path"),
                            lambda v: self._set("_file_path", v),
                            {},
                        ),
                        "prefix": (
                            self._get("_file_prefix"),
                            lambda v: self._set("_file_prefix", v),
                            {},
                        ),
                        "extension": (
                            self._get("_file_extension"),
                            lambda v: self._set("_file_extension", v),
                            {},
                        ),
                    },
                    "write": (lambda: False, self.execute_write, {})
                },
            },
            mutable=True,
        )

    def initialize(self, odin_adapter_frs, odin_adapter_fps):
        # Record the references to the OdinDataAdapters
        # for the FrameReceivers and FrameProcessors.
        self._odin_adapter_frs = odin_adapter_frs
        self._odin_adapter_fps = odin_adapter_fps

        self._odin_adapter_fps._controller.merge_external_tree("config", self._fp_params)

        # Set up the rank of the individual FP applications
        # This must be called after the adapters have started
        self._thread = threading.Thread(target=self.init_rank)
        self._thread.start()

    def execute_write(self, value):
        # Queue the write command
        logging.error("Writing command: {}".format(value))

        if value:
            # Before attempting to write files, make some simple error checks

            # Check if we have a valid buffer status from the FR adapter

            # TODO: Need to check FR buffer status
#            valid, reason = self.check_fr_status()
#            if not valid:
#                raise RuntimeError(reason)

            # Check the file prefix is not empty
            if str(self._file_prefix) == '':
                raise RuntimeError("File prefix must not be empty")

            # First setup the rank for the frameProcessor applications
            self.setup_rank()

            processes = self._odin_adapter_fps._controller.get("count", False)["count"]
            try:
                for rank in range(processes):
                    # Setup the number of processes and the rank for each client
                    config = {
                        'hdf': {
                            'frames': self._frames
                        }
                    }
                    logging.error("Sending config to FP odin adapter %i: %s", rank, config)
                    self._odin_adapter_fps._controller.put(f"{rank}/config", config)
                    config = {
                        'hdf': {
                            'acquisition_id': self._acquisition_id,
                            'file': {
                                'path': str(self._file_path),
                                'name': str(self._file_prefix),
                                'extension': str(self._file_extension)
                            }
                        }
                    }
                    logging.error("Sending config to FP odin adapter %i: %s", rank, config)
                    self._odin_adapter_fps._controller.put(f"{rank}/config", config)
            except Exception as err:
                logging.debug("Failed to send rank information to FP applications")
                logging.error("Error: %s", err)
        try:
            config = {'hdf': {'write': value}}
            for rank in range(processes):
                logging.error("Sending config to FP odin adapter %i: %s", rank, config)
                self._odin_adapter_fps._controller.put(f"{rank}/config", config)
        except Exception as err:
            logging.debug("Failed to send write command to FP applications")
            logging.error("Error: %s", err)

    def init_rank(self):
        # Send the setup rank after allowing time for the
        # other adapters to run up.
        while not self._odin_adapter_fps._controller.first_update:
            time.sleep(0.1)
        self.setup_rank()

    def _set(self, attr, val):
        logging.debug("_set called: {}  {}".format(attr, val))
        setattr(self, attr, val)

    def _get(self, attr):
        return lambda: getattr(self, attr)

    def setup_rank(self):
        # Attempt initialisation of the connected clients
        processes = self._odin_adapter_fps._controller.get("count", False)["count"]
        logging.info(
            "Setting up rank information for {} FP processes".format(processes)
        )
        rank = 0
        try:
            for rank in range(processes):
                # Setup the number of processes and the rank for each client
                config = {"hdf": {"process": {"number": processes, "rank": rank}}}
                logging.debug("Sending config to FP odin adapter %i: %s", rank, config)
                self._odin_adapter_fps._controller.put(f"{rank}/config", config)

        except Exception as err:
            logging.debug("Failed to send rank information to FP applications")
            logging.error("Error: %s", err)
