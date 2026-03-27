import pytest
from time import sleep
from odin_data.client import OdinDataClient
from system_under_test import SystemUnderTest

def test_can_start_frame_processor(sut_launch):
   sut = SystemUnderTest()
   sut.set_cfg("frameProcessor", {"ctrl": 5004})
   sut_launch(sut)
   status = sut.client("frameProcessor").do_status_cmd()
   assert status["msg_val"] == "status"

def test_can_start_frame_receiver(sut_launch):
   sut = SystemUnderTest()
   sut.set_cfg("frameReceiver", {"ctrl": 5000})
   sut_launch(sut)
   status = sut.client("frameReceiver").do_status_cmd()
   assert status["msg_val"] == "status"

def test_can_start_frame_simulator(sut_launch):
   sut = SystemUnderTest()
   sut.set_cfg("frameSimulator", {"decoder": "DummyUDP", "dest-ip": "127.0.0.1", "frames": 10, "ports": "61649", "packet-gap": 10})
   sut_launch(sut)

def test_can_start_receiver_and_processor(sut_launch):
   sut = SystemUnderTest()
   sut.set_cfg("frameReceiver", {"ctrl": 5000})
   sut.set_cfg("frameProcessor", {"ctrl": 5004})
   sut_launch(sut)
   status = sut.client("frameReceiver").do_status_cmd()
   assert status["msg_val"] == "status"
   status = sut.client("frameProcessor").do_status_cmd()
   assert status["msg_val"] == "status"

def test_simple_pipe(sut_launch):
   # Trying to mimic odinDataTest, but not working yet!
   sut = SystemUnderTest()
   sut.set_cfg("frameReceiver", {"ctrl": 5000, "config": "dummyUDP-fr.json"})
   sut.set_cfg("frameProcessor", {"ctrl": 5004, "config": "dummyUDP-fp.json"})
   sut.set_cfg("frameSimulator", {"decoder": "DummyUDP", "dest-ip": "127.0.0.1", "frames": 10, "ports": "61649", "packet-gap": 10})
   sut_launch(sut)

# Running /atsw-work/odin-dev-release/bin/frameSimulator DummyUDP --lib-path=/atsw-work/odin-dev-release/lib --frames=10 --dest-ip=127.0.0.1 --ports=61649 --packet-gap=10
