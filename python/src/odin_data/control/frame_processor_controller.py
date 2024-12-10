import logging

from odin_data.control.odin_data_controller import OdinDataController


class FrameProcessorController(OdinDataController):
    def __init__(self, name, endpoints, update_interval=0.5):
        super().__init__(name, endpoints, update_interval)
        # If we are a FP controller then we need to track the writing state
        self._first_update = False
        self._writing = [False]*len(self._clients)

    @property
    def first_update(self):
        return self._first_update

    def setup_parameter_tree(self):
        super(FrameProcessorController, self).setup_parameter_tree()
        self._acquisition_id = ""
        self._write = False
        self._frames = 0
        self._file_path = ""
        self._file_prefix = ""
        self._file_extension = "h5"
        self._tree["config"] = {
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
                "write": (
                    self._get("_write"),
                     self.execute_write,
                      {}
                    )
            },
        }

    def execute_write(self, value):
        # Queue the write command
        logging.debug("Executing write command with value: {}".format(value))
        processes = len(self._clients)

        if value:
            # Before attempting to write files, make some simple error checks

            # Check if we have a valid buffer status from the FR adapter

            # valid, reason = self.check_controller_status()
            # if not valid:
            #     raise RuntimeError(reason)

            # Check the file prefix is not empty
            if str(self._file_prefix) == '':
                raise RuntimeError("File prefix must not be empty")

            # First setup the rank
            self.setup_rank()

            try:
                for rank in range(processes):
                    # Setup the number of processes and the rank for each client
                    config = {
                        'hdf': {
                            'frames': self._frames
                        }
                    }
                    logging.info("Sending config to FP odin adapter %i: %s", rank, config)
                    self._clients[rank].send_configuration(config)
                    config = {
                        'hdf': {
                            'acquisition_id': self._acquisition_id,
                            'file': {
                                'path': str(self._file_path),
                                'prefix': str(self._file_prefix),
                                'extension': str(self._file_extension)
                            }
                        }
                    }
                    logging.info("Sending config to FP odin adapter %i: %s", rank, config)
                    self._clients[rank].send_configuration(config)
            except Exception as err:
                logging.error("Failed to send information to FP applications")
                logging.error("Error: %s", err)
        try:
            config = {'hdf': {'write': value}}
            for rank in range(processes):
                logging.info("Sending config to FP odin adapter %i: %s", rank, config)
                #self._odin_adapter_fps._controller.put(f"{rank}/config", config)
                self._clients[rank].send_configuration(config)
        except Exception as err:
            logging.error("Failed to send write command to FP applications")
            logging.error("Error: %s", err)

    def handle_client(self, client, index):
        if "hdf" in client.parameters["status"]:
            self._writing[index] = client.parameters["status"]["hdf"]["writing"]

    def setup_rank(self):
        # Attempt initialisation of the connected clients
        processes = len(self._clients)
        logging.info(
            "Setting up rank information for {} FP processes".format(processes)
        )
        rank = 0
        try:
            for rank in range(processes):
                # Setup the number of processes and the rank for each client
                config = {"hdf": {"process": {"number": processes, "rank": rank}}}
                logging.debug("Sending config to FP odin adapter %i: %s", rank, config)
                self._clients[rank].send_configuration(config)

        except Exception as err:
            logging.debug("Failed to send rank information to FP applications")
            logging.error("Error: %s", err)

    def process_updates(self):
        if not self._first_update:
            self.setup_rank()
            self._first_update = True
        self._write = all(self._writing)

    # def check_controller_status(self):
        # TODO: Need to check FR buffer status
        # return True, ""
