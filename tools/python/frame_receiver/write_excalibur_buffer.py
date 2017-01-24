from __future__ import print_function

import os, time
import npyscreen
import argparse
import zmq
import json
import curses
from ipc_channel import IpcChannel
from ipc_message import IpcMessage

from frame_receiver.shared_buffer_manager import SharedBufferManager, SharedBufferManagerException
from struct import Struct

shared_mem_name = "ExcaliburSharedBuffer"
#buffer_size     = 1048576
num_buffers     = 10
boost_mmap_mode = False

def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-p", "--path", default="/home/gnx91527/work/percival/datasets", help="Raw data file")
    parser.add_argument("--ready", default="tcp://127.0.0.1:5001", help="Frame Ready endpoint")
    parser.add_argument("--release", default="tcp://127.0.0.1:5002", help="Frame Release endpoint")
    parser.add_argument("-b", "--buffer", default=shared_mem_name, help="Unique name of shared buffer")
    args = parser.parse_args()
    return args

class ExcaliburTestApp(npyscreen.NPSAppManaged):
    def __init__(self, ready_endpoint, release_endpoint, buffer, filepath):
        super(ExcaliburTestApp, self).__init__()
        self._ready_endpoint = ready_endpoint
        self._release_endpoint = release_endpoint
        self._buffer = buffer
        self._ctrl_channel = None
        self._release_channel = None
        self._current_value = None
        self._prev_value = None
        self._no_of_frames = 1
        self._frame_rate = 1.0
        self._running = False
        self._frames_sent = 0
        self._last_millis = 0
        self._filename = "excalibur-test-12bit.raw"
        self._filepath = filepath
        self._frames = 1

    def onStart(self):
        self.keypress_timeout_default = 1
        self.registerForm("MAIN", IntroForm())
        self.registerForm("MAIN_MENU", MainMenu())
        self.registerForm("SETUP", SetupAcquisition())
        
    def send_message(self, ipc_message):
        self._ctrl_channel.send(ipc_message.encode())
        
    def setup_buffer(self, filename, buffer_size, frames):
        self._filename = filename
        self._frames = frames
        # Create the shared buffer
        self._shared_buffer_manager = SharedBufferManager(self._buffer,
                                                          buffer_size*num_buffers,
                                                          buffer_size, 
                                                          remove_when_deleted=True, 
                                                          boost_mmap_mode=boost_mmap_mode)
        # Now load in the file
        with open(self._filepath + "/" + self._filename, mode='rb') as file:
            fileContent = file.read()
            print("Writing file content to ", shared_mem_name)
            self._shared_buffer_manager.write_buffer(0, fileContent)
        

    def read_message(self, timeout):
        pollevts = self._ctrl_channel.poll(timeout)
        if pollevts == zmq.POLLIN:
            reply = IpcMessage(from_str=self._ctrl_channel.recv())
            return reply
        return None

# This form class defines the display that will be presented to the user.

class IntroForm(npyscreen.Form):
    def create(self):
        self.name = "Excalibur test application"

        self.add(npyscreen.TitleText, labelColor="LABELBOLD", name="Set the frame ready endpoint for this test", value="", editable=False)
        self.ready = self.add(npyscreen.TitleText, name="Frame Notify Endpoint: ", value="")

        self.add(npyscreen.TitleText, labelColor="LABELBOLD", name=" ", value="", editable=False)

        self.add(npyscreen.TitleText, labelColor="LABELBOLD", name="Set the frame release endpoint for this test", value="", editable=False)
        self.release = self.add(npyscreen.TitleText, name="Frame Release Endpoint: ", value="")

        self.add(npyscreen.TitleText, labelColor="LABELBOLD", name=" ", value="", editable=False)

        self.add(npyscreen.TitleText, labelColor="LABELBOLD", name="Set the unique name for the shared memory buffer", value="", editable=False)
        self.buffer = self.add(npyscreen.TitleText, name="Shared memory buffer name: ", value="")

        self.add(npyscreen.TitleText, labelColor="LABELBOLD", name=" ", value="", editable=False)

        self.add(npyscreen.TitleText, labelColor="LABELBOLD", name="Which type of dataset (1bit, 6bit, 12bit, 24bit)", value="", editable=False)
        self.datatype = self.add(npyscreen.TitleText, name="Datatype: ", value="12bit")

    def beforeEditing(self):
        self.ready.value = self.parentApp._ready_endpoint
        self.release.value = self.parentApp._release_endpoint
        self.buffer.value = self.parentApp._buffer

    def afterEditing(self):
        self.parentApp._ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_PUB)
        self.parentApp._ctrl_channel.bind(self.ready.value)
        self.parentApp._release_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_SUB)
        self.parentApp._release_channel.bind(self.release.value)
        if self.datatype.value == "1bit":
          self.parentApp.setup_buffer("excalibur-test-1bit.raw", 65536, 1)
        elif self.datatype.value == "6bit":
          self.parentApp.setup_buffer("excalibur-test-6bit.raw", 524288, 1)
        elif self.datatype.value == "12bit":
          self.parentApp.setup_buffer("excalibur-test-12bit.raw", 1048576, 1)
        elif self.datatype.value == "24bit":
          self.parentApp.setup_buffer("excalibur-test-24bit.raw", 1048576, 2)
        else:
          self.parentApp.setNextForm(None)

        self.parentApp.setNextForm("MAIN_MENU")
        


class MainMenu(npyscreen.FormBaseNew):
    def create(self):
        self.keypress_timeout = 1
        self.name = "Excalibur test application"
        self.t2 = self.add(npyscreen.BoxTitle, name="Main Menu:", relx=2, max_width=24) #, max_height=20)
        self.t3 = self.add(npyscreen.BoxTitle, name="Response:", rely=2, relx=26) #, max_width=45, max_height=20)
        
        self.t2.values = ["Setup Acquisition",
                          "Run Acquisition",
                          "Exit"]
        self.t2.when_value_edited = self.button

    def button(self):
        selected = self.t2.entry_widget.value
        if selected == 0:
          self.parentApp.setNextForm("SETUP")
          self.parentApp.switchFormNow()
        if selected == 1:
          self.parentApp._frames_sent = 0
          self.parentApp._running = True              
        if selected == 2:
          self.parentApp.setNextForm(None)
          self.parentApp.switchFormNow()
          
    def while_waiting(self):
        if self.parentApp._running == True:
            millis = int(round(time.time() * 1000))
            if millis > (self.parentApp._last_millis + int(1000 / self.parentApp._frame_rate)):
                msg = IpcMessage("notify", "frame_ready")
                msg.set_param("frame", self.parentApp._frames_sent)
                buff_id = self.parentApp._frames_sent % self.parentApp._frames
                msg.set_param("buffer_id", buff_id)
                self.parentApp.send_message(msg)
                self.parentApp._frames_sent = self.parentApp._frames_sent + 1
                self.parentApp._last_millis = millis
                if self.parentApp._frames_sent == self.parentApp._no_of_frames:
                    self.parentApp._running = False
        self.t3.values = ["Running: " + str(self.parentApp._running),
                          "Frames:  " + str(self.parentApp._frames_sent)]
        self.t3.display()
        self.t2.entry_widget.value = None
        self.t2.entry_widget._old_value = None
        self.t2.display()

class SetupAcquisition(npyscreen.ActionForm):
    def create(self):
        self.name = "Excalibur test application"
        self.add(npyscreen.TitleText, labelColor="LABELBOLD", name="Setup acquisition parameters", value="", editable=False)
        self.ctrl1 = self.add(npyscreen.TitleText, name="Number of frames: ", value="1")
        self.ctrl2 = self.add(npyscreen.TitleText, name="Frame rate (# per second):   ", value="1.0")

    def on_ok(self):
        self.parentApp._no_of_frames = int(self.ctrl1.value)
        self.parentApp._frame_rate = float(self.ctrl2.value)

    def afterEditing(self):
        self.parentApp.setNextForm("MAIN_MENU")
        self.editing = False
        self.parentApp.switchFormNow()



def main():
    args = options()

    app = ExcaliburTestApp(args.ready, args.release, args.buffer, args.path)
    app.run()


if __name__ == '__main__':
    main()

