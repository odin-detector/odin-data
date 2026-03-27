import subprocess

from pathlib import Path
from odin_data.client import OdinDataClient

class SystemUnderTest:
    def __init__(self):
        self._install_path = None
        self._cfg = {"frameProcessor": None, "frameReceiver": None, "frameSimulator": None}
        self._proc = {"frameProcessor": None, "frameReceiver": None, "frameSimulator": None}
        self._client = {"frameProcessor": None, "frameReceiver": None, "frameSimulator": None}

    @property
    def install_path(self):
        assert self._install_path is not None
        return self._install_path

    @install_path.setter
    def install_path(self, path):
        assert isinstance(path, Path), "The install path must be a Path"
        self._install_path = path

    def cfg(self, app):
        return self._cfg[app]

    def set_cfg(self, app, cfg):
        assert app in self._cfg, app
        assert isinstance(cfg, dict), "The cfg must be a dict"
        self._cfg[app] = cfg

    def proc(self, app):
        return self._proc[app]

    def set_proc(self, app, proc):
        assert app in self._proc, app
        assert self._proc[app] is None, "Process can only be launched once"
        assert isinstance(proc, subprocess.Popen), "The proc must be a subprocess.Popen"
        self._proc[app] = proc

    def client(self, app):
        return self._client[app]

    def set_client(self, app, client):
        assert app in self._client, app
        assert isinstance(client, OdinDataClient), "The client must be an OdinDataClient"
        self._client[app] = client

    def cleanup(self):
        def check_proc_status(app, proc):
            if proc.returncode != 0:
                for line in proc.stderr:
                    linestr = line.decode("utf-8").rstrip()
                    print(f"{app}:stderr:{linestr}")
                return False
            # Need to check the stdout for ERROR as at least frameSimulator
            # produces errors but still exits with status 0.
            # DIAMOND_TODO #490: frameSimulator should be changed so this is not
            # required as this is horrible.
            error_lines = []
            for line in proc.stdout:
                linestr = line.decode("utf-8").rstrip()
                print(linestr)
                if " ERROR " in linestr:
                    error_lines.append(linestr)
            return len(error_lines) == 0

        print("Test complete, cleaning up")
        cleanup_ok = True
        for app, client in self._client.items():
            if client is not None:
                print(f"Instructing {app} to shutdown")
                status = client.do_shutdown_cmd()
                if status['msg_type'] != 'ack' or status['msg_val'] != 'shutdown':
                    print(f"Warning: {app} shutdown failed: {status}")
                    cleanup_ok = False
        for app, proc in self._proc.items():
            if proc is not None:
                print(f"Waiting for {app} to complete")
                proc.wait()

        procs_ok = True
        for app, proc in self._proc.items():
            if proc is not None:
                if not check_proc_status(app, proc):
                    procs_ok = False
        assert procs_ok, "Test processes failed"

        assert cleanup_ok, "Test cleanup failed"
