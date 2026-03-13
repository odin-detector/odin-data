import json
import os
import subprocess
import sys
import time
from pathlib import Path

import h5py as h5
import zmq
from odin_data.control.ipc_message import IpcMessage


class TestMetaWriter:
    def test_full_stack_meta_writer(self):
        # Spawn the meta_writer application into a subprocess
        subprocess_path = str(
            Path(__file__).parent.parent
            / "src/odin_data/meta_writer/meta_writer_app.py"
        )
        self.process = subprocess.Popen(
            [sys.executable, "-m", "coverage", "run", subprocess_path]
        )

        # Create the control socket and data socket
        context = zmq.Context()
        ctrl_endpoint = "tcp://127.0.0.1:{}".format(5659)
        ctrl_socket = context.socket(zmq.DEALER)
        ctrl_socket.connect(ctrl_endpoint)

        data_endpoint = "tcp://127.0.0.1:{}".format(5558)
        data_socket = context.socket(zmq.PUB)
        data_socket.bind(data_endpoint)

        msg_id = 0

        msg_id += 1
        msg = IpcMessage(IpcMessage.ACK, "configure", id=msg_id)
        msg.set_params({"acquisition_id": "test1", "file_prefix": "test_file_1"})
        ctrl_socket.send_string(msg.encode())
        reply = IpcMessage(from_str=ctrl_socket.recv())
        assert reply.get_msg_type() == IpcMessage.ACK

        msg_id += 1
        msg = IpcMessage(IpcMessage.ACK, "configure", id=msg_id)
        msg.set_params({"acquisition_id": "test2", "file_prefix": "test_file_2"})
        ctrl_socket.send_string(msg.encode())
        reply = IpcMessage(from_str=ctrl_socket.recv())
        assert reply.get_msg_type() == IpcMessage.ACK

        msg_id += 1
        msg = IpcMessage(IpcMessage.ACK, "request_configuration", id=msg_id)
        ctrl_socket.send_string(msg.encode())
        reply = IpcMessage(from_str=ctrl_socket.recv())
        assert reply.get_msg_type() == IpcMessage.ACK
        print("{}".format(reply.get_params()))
        assert "test1" in reply.get_params()["acquisitions"]
        assert (
            reply.get_params()["acquisitions"]["test1"]["file_prefix"] == "test_file_1"
        )
        assert "test2" in reply.get_params()["acquisitions"]
        assert (
            reply.get_params()["acquisitions"]["test2"]["file_prefix"] == "test_file_2"
        )

        msg_id += 1
        msg = IpcMessage(IpcMessage.ACK, "status", id=msg_id)
        ctrl_socket.send_string(msg.encode())
        reply = IpcMessage(from_str=ctrl_socket.recv())
        print("{}".format(reply.get_params()))
        assert reply.get_msg_type() == IpcMessage.ACK

        data_socket.send_multipart(
            [
                json.dumps(
                    {
                        "header": {"acqID": "test2", "totalFrames": 1},
                        "type": "str",
                        "parameter": "startacquisition",
                    }
                ).encode(),
                "".encode(),
            ]
        )

        # This message will generate an error and the frame count is ignored
        data_socket.send_multipart(
            [
                json.dumps(
                    {
                        "header": {"acqID": "test2", "totalFrames": 5},
                        "type": "str",
                        "parameter": "startacquisition",
                    }
                ).encode(),
                "".encode(),
            ]
        )
        data_socket.send_multipart(
            [
                json.dumps(
                    {
                        "header": {"acqID": "test2"},
                        "type": "int",
                        "parameter": "createfile",
                    }
                ).encode(),
                '{"create_duration":20}'.encode(),
            ]
        )
        data_socket.send_multipart(
            [
                json.dumps(
                    {
                        "header": {"acqID": "test2"},
                        "type": "int",
                        "parameter": "writeframe",
                    }
                ).encode(),
                '{"frame":0, "offset":0, "write_duration":25, "flush_duration":30}'.encode(),
            ]
        )
        data_socket.send_multipart(
            [
                json.dumps(
                    {
                        "header": {"acqID": "test2"},
                        "type": "int",
                        "parameter": "closefile",
                    }
                ).encode(),
                '{"close_duration":35}'.encode(),
            ]
        )
        data_socket.send_multipart(
            [
                json.dumps(
                    {
                        "header": {"acqID": "test2"},
                        "type": "int",
                        "parameter": "stopacquisition",
                    }
                ).encode(),
                "".encode(),
            ]
        )
        time.sleep(1.0)

        msg_id += 1
        msg = IpcMessage(IpcMessage.ACK, "shutdown", id=msg_id)
        ctrl_socket.send_string(msg.encode())

        data_socket.close()

        # Once the application has closed down, open the HDF5 file and verify contents
        with h5.File("/tmp/test_file_2_meta.h5", "r") as f:
            assert f["create_duration"][0] == 20
            assert f["write_duration"][0] == 25
            assert f["flush_duration"][0] == 30
            assert f["close_duration"][0] == 35
            assert f["frame"][0] == 0
            assert f["offset"][0] == 0

        # Remove the temporary file
        os.remove("/tmp/test_file_2_meta.h5")
