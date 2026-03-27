import pytest
from time import sleep
from odin_data.client import OdinDataClient

@pytest.mark.parametrize("frame_processor", [{"ctrl": 5004}], indirect=True)
def test_can_start_frame_processor(frame_processor):
   fp, fpclient, fpparams = frame_processor
   status = fpclient.do_status_cmd()
   assert status["msg_val"] == "status"

@pytest.mark.parametrize("frame_receiver", [{"ctrl": 5000}], indirect=True)
def test_can_start_frame_receiver(frame_receiver):
   fr, frclient, frparams = frame_receiver
   status = frclient.do_status_cmd()
   assert status["msg_val"] == "status"

@pytest.mark.parametrize("frame_simulator", [{"decoder": "DummyUDP", "dest-ip": "127.0.0.1", "frames": 5}], indirect=True)
def test_can_start_frame_simulator(frame_simulator):
   fs, _, fsparams = frame_simulator

@pytest.mark.parametrize("frame_simulator", [{"decoder": "DummyUDP", "dest-ip": "127.0.0.1", "frames": 10, "ports": "61649", "packet-gap": 10}], indirect=True)
@pytest.mark.parametrize("frame_receiver", [{"ctrl": 5000, "config": "dummyUDP-fr.json"}], indirect=True)
@pytest.mark.parametrize("frame_processor", [{"ctrl": 5004, "config": "dummyUDP-fp.json"}], indirect=True)
def test_simple_pipe(frame_simulator, frame_receiver, frame_processor):
   # Trying to mimic odinDataTest, but not working yet!
   fr, frclient, frparams = frame_receiver
   fp, fpclient, fpparams = frame_processor
   fs, _, fsparams = frame_simulator


# Running /atsw-work/odin-dev-release/bin/frameSimulator DummyUDP --lib-path=/atsw-work/odin-dev-release/lib --frames=10 --dest-ip=127.0.0.1 --ports=61649 --packet-gap=10
