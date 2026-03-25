import os
import pytest
import shlex
import subprocess
from pathlib import Path
from time import sleep

from odin_data.client import OdinDataClient

@pytest.fixture
def install_prefix():
   try:
      iprefix = os.environ["INSTALL_PREFIX"]
   except KeyError:
      raise KeyError("INSTALL_PREFIX is not set")
   assert Path(iprefix).exists(), f"INSTALL_PREFIX: {iprefix} not found"
   return iprefix

def app_startup(install_path, app, param, msg_prefix):
   app_path = install_path / "bin" / app
   cmd = [app_path.as_posix()]
   ctrl_endpoint = None
   for k, v in param.items():
      assert k in ["ctrl", "config", "debug-level", "decoder", "lib-path", "frames", "dest-ip", "ports", "packet-gap", "packet-len", "width", "height", "drop-fraction", "drop-packets", "interval", "pcap-file"], k
      if k == "ctrl":
         assert isinstance(v, int), f"{msg_prefix} {k} argument must be type int"
         ctrl_endpoint = f"tcp://127.0.0.1:{v}"
         cmd.append(f"--{k} {ctrl_endpoint}")
      elif k == "config":
         assert isinstance(v, str), f"{msg_prefix} {k} argument must be type str"
         cpath = install_path / "test_config" / v
         assert cpath.exists(), f"{msg_prefix} {k} path {cpath} not found"
         cmd.append(f"--{k} {cpath.as_posix()}")
      elif k == "lib-path":
         assert isinstance(v, str), f"{msg_prefix} {k} argument must be type str"
         lpath = install_path / v
         assert lpath.exists(), f"{msg_prefix} {k} path {lpath} not found"
         cmd.append(f"--{k} {lpath.as_posix()}")
      elif k == "decoder":
         assert isinstance(v, str), f"{msg_prefix} {k} argument must be type str"
         # Insert a positional arg
         cmd.insert(1, v)
      elif k in ["debug-level", "frames", "packet-gap", "packet-len", "width", "height"]: # Generic int parameter
         assert isinstance(v, int), f"{msg_prefix} {k} argument must be type int"
         cmd.append(f"--{k} {v}")
      elif k in ["dest-ip", "drop-packets", "pcap-file", "ports"]: # Generic str parameter
         assert isinstance(v, str), f"{msg_prefix} {k} argument must be type str"
         cmd.append(f"--{k} {v}")
      elif k in ["drop-fraction", "interval"]: # Generic float parameter
         assert isinstance(v, str), f"{msg_prefix} {k} argument must be type str"
         cmd.append(f"--{k} {v}")
      else:
         assert False, f"{msg_prefix} Unknown parameter name {k}"

   cmdstring = " ".join(cmd)
   print(f"Launching: {cmdstring}")
   proc = subprocess.Popen(shlex.split(cmdstring), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
   client = None
   if ctrl_endpoint is not None:
      client = OdinDataClient(is_cli=False, ctrl_endpoint=ctrl_endpoint)
      # Poll for app being up by checking that status is not None
      retries = 5
      is_up = False
      while retries != 0:
         if client.do_status_cmd() is not None:
            is_up = True
            break
         print(f"Waiting for {app_path.name} to start up")
         retries -= 1
         sleep(1)
   return proc, client

def app_teardown(proc, client):
   if client is not None:
      client.do_shutdown_cmd()
   proc.wait()
   assert proc.returncode == 0, f"Error: {proc.stderr}"
   # Need to check the stdout for ERROR as at least frameSimulator produces errors but still
   # exits with status 0.
   # DIAMOND_TODO frameSimulator should be changed so this is not required as this is horrible.
   error_lines = []
   for line in proc.stdout:
      linestr = line.decode("utf-8").rstrip()
      print(linestr)
      if " ERROR " in linestr:
         error_lines.append(linestr)
   assert len(error_lines) == 0, f"Error: found {len(error_lines)} error in stdout\n{error_lines}"

@pytest.fixture
def frame_processor(request, install_prefix):
   install_path = Path(install_prefix)
   msg_prefix = f"Fixture {request.fixturename}:"
   assert hasattr(request, "param"), f"{msg_prefix} requires param"
   assert isinstance(request.param, dict), f"{msg_prefix} param must be a dict"
   assert "ctrl" in request.param, f"{msg_prefix} requires ctrl to be set"
   print(f"{msg_prefix} using {request.param}")
   proc, client = app_startup(install_path, "frameProcessor", request.param, msg_prefix)
   # These must be yielded rather than returned so that the teardown can happen once the test
   # is complete
   yield proc, client, request.param
   app_teardown(proc, client)

@pytest.fixture
def frame_receiver(request, install_prefix):
   install_path = Path(install_prefix)
   msg_prefix = f"Fixture {request.fixturename}:"
   assert hasattr(request, "param"), f"{msg_prefix} requires param"
   assert isinstance(request.param, dict), f"{msg_prefix} param must be a dict"
   assert "ctrl" in request.param, f"{msg_prefix} requires ctrl to be set"
   print(f"{msg_prefix} using {request.param}")
   # These must be yielded rather than returned so that the teardown can happen once the test
   # is complete
   proc, client = app_startup(install_path, "frameReceiver", request.param, msg_prefix)
   yield proc, client, request.param
   app_teardown(proc, client)

@pytest.fixture
def frame_simulator(request, install_prefix):
   install_path = Path(install_prefix)
   msg_prefix = f"Fixture {request.fixturename}:"
   assert hasattr(request, "param"), f"{msg_prefix} requires param"
   assert isinstance(request.param, dict), f"{msg_prefix} param must be a dict"
   # This must have a decoder, this is a positional arg
   assert "decoder" in request.param, f"{msg_prefix} requires decoder to be set"
   # This must set the number of frames, otherwise the app will not terminate
   assert "frames" in request.param, f"{msg_prefix} requires frames to be set"
   assert "lib-path" not in request.param
   request.param["lib-path"] = "lib"
   print(f"{msg_prefix} using {request.param}")
   proc, client = app_startup(install_path, "frameSimulator", request.param, msg_prefix)
   yield proc, client, request.param
   app_teardown(proc, client)
