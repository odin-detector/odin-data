import os

from common import ZMQClient


ODIN_DATA_DIR = "/dls_sw/work/tools/RHEL6-x86_64/odin/odin-data/build"
EIGER_DIR = "/dls_sw/work/tools/RHEL6-x86_64/odin/eiger-daq/build-dir"


class FrameProcessorClient(ZMQClient):

    CTRL_PORT = 5004
    META_PORT = 5558
    META_ENDPOINT = "tcp://*:{}"

    PLUGIN = "plugin"
    FILE_WRITER = "hdf"
    SHARED_MEMORY_SETUP = "fr_setup"
    META = "meta_endpoint"
    INPUT = "frame_receiver"
    EIGER = "eiger"
    LIBRARIES = {EIGER: os.path.join(EIGER_DIR, "lib")}

    def __init__(self, rank, processes, ip_address, lock, server_rank=0):
        super(FrameProcessorClient, self).__init__(ip_address, lock,
                                                   server_rank)
        self.processes = processes
        self.rank = rank

        self.meta_endpoint = self.META_ENDPOINT.format(self.META_PORT +
                                                       1000 * server_rank)

    def request_status(self):
        response = self.send_request("status")
        return response.attrs["params"]

    def request_frames_written(self):
        status = self.request_status()
        return status[self.FILE_WRITER]["frames_written"]

    def request_frames_expected(self):
        status = self.request_status()
        return status[self.FILE_WRITER]["frames_max"]

    def stop(self):
        config = {
            "write": False
        }
        self.send_configuration(self.FILE_WRITER, config)

    def configure_shared_memory(self, shared_memory, ready, release):
        config = {
            "fr_shared_mem": shared_memory,
            "fr_ready_cnxn": ready,
            "fr_release_cnxn": release
        }
        self.send_configuration(self.SHARED_MEMORY_SETUP, config)

    def configure_meta(self):
        self.send_configuration(self.META, self.meta_endpoint)

    def load_plugin(self, plugin):
        library, name = self.parse_plugin_definition(plugin,
                                                     self.LIBRARIES[plugin])
        config = {
            "load": {
                "library": library,
                "index": plugin,
                "name": name,
            }
        }
        self.send_configuration(self.PLUGIN, config)

    def load_file_writer_plugin(self, index):
        config = {
            "load": {
                "library": os.path.join(ODIN_DATA_DIR, "lib/libHdf5Plugin.so"),
                "index": index,
                "name": "FileWriterPlugin",
            },
        }
        self.send_configuration(self.PLUGIN, config)

    def connect_plugins(self, source, sink):
        config = {
            "connect": {
                "connection": source,
                "index": sink
            }
        }
        self.send_configuration(self.PLUGIN, config)

    def disconnect_plugins(self, source, sink):
        config = {
            "disconnect": {
                "index": source,
                "connection": sink
            }
        }
        self.send_configuration(self.PLUGIN, config)

    def configure_file(self, path, name, frames, acq_id=None):
        config = {
            "file": {
                "path": path,
                "name": name,
            },
            "frames": int(frames),
            "write": True
        }
        if acq_id is not None:
            config["acquisition_id"] = acq_id
        self.send_configuration(self.FILE_WRITER, config)

    def create_dataset(self, name, dtype, dimensions,
                       chunks=None, compression=None):
        config = {
            "dataset": {
                "cmd": "create",
                "name": name,
                "datatype": int(dtype),
                "dims": dimensions
            }
        }
        if chunks is not None:
            config["dataset"]["chunks"] = chunks
        if compression is not None:
            config["dataset"]["compression"] = int(compression)
        self.send_configuration(self.FILE_WRITER, config)

    def configure_file_process(self):
        config = {
            "process": {
                "number": int(self.processes),
                "rank": int(self.rank)
            },
        }
        self.send_configuration(self.FILE_WRITER, config)

    def configure_frame_count(self, frames):
        config = {
            "frames": frames
        }
        self.send_configuration(self.FILE_WRITER, config)

    def rewind(self, frames, active_frame):
        config = {
            "rewind": int(frames),
            "rewind/active_frame": int(active_frame)
        }
        self.send_configuration(self.FILE_WRITER, config)

    def set_initial_frame(self, frame):
        config = {
            "initial_frame": int(frame)
        }
        self.send_configuration(self.FILE_WRITER, config)

    @staticmethod
    def parse_plugin_definition(name, path):
        library_name = "{}ProcessPlugin".format(name.capitalize())
        library_path = os.path.join(path, "lib{}.so".format(library_name))

        return library_path, library_name
