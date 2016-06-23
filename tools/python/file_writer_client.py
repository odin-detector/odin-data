from __future__ import print_function

import os, time
import argparse
import zmq
import json
import curses

import os
import npyscreen
from ipc_channel import IpcChannel
from ipc_message import IpcMessage

class PercivalClientApp(npyscreen.NPSAppManaged):
    def __init__(self, ctrl_endpoint):
        super(PercivalClientApp, self).__init__()
        self._ctrl_endpoint = ctrl_endpoint
        self._poller = zmq.Poller()
        self._ctrl_channel = None
        self._current_value = None
        self._prev_value = None

    def onStart(self):
        self.keypress_timeout_default = 1
        self.registerForm("MAIN", IntroForm())
        self.registerForm("MAIN_MENU", MainMenu())
        self.registerForm("LOAD_PLUGIN", LoadPlugin())
        self.registerForm("SETUP_FILE", SetupFileWriting())
        
    def send_message(self, ipc_message):
        self._ctrl_channel.send(ipc_message.encode())
        pollevts = self._ctrl_channel.poll(1000)
        if pollevts == zmq.POLLIN:
            reply = IpcMessage(from_str=self._ctrl_channel.recv())
            if reply:
                self._current_value = str(reply)
        

    def read_message(self, timeout):
        pollevts = self._ctrl_channel.poll(timeout)
        if pollevts == zmq.POLLIN:
            reply = IpcMessage(from_str=self._ctrl_channel.recv())
            return reply
        return None

# This form class defines the display that will be presented to the user.

class IntroForm(npyscreen.Form):
    def create(self):
        self.name = "ODIN File Writer Client"
        self.add(npyscreen.TitleText, labelColor="LABELBOLD", name="Set the control endpoint for the application", value="", editable=False)
        self.ctrl = self.add(npyscreen.TitleText, name="Control Endpoint: ", value="")

    def beforeEditing(self):
        self.ctrl.value = self.parentApp._ctrl_endpoint

    def afterEditing(self):
        self.parentApp._ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_PAIR)
        self.parentApp._ctrl_channel.connect(self.ctrl.value)
        self.parentApp.setNextForm("MAIN_MENU")


class MainMenu(npyscreen.FormBaseNew):
    def create(self):
        self.keypress_timeout = 1
        self.name = "ODIN File Writer Client"
        self.t2 = self.add(npyscreen.BoxTitle, name="Main Menu:", relx=2, max_width=24) #, max_height=20)
        self.t3 = self.add(npyscreen.BoxTitle, name="Response:", rely=2, relx=26) #, max_width=45, max_height=20)
        
        self.t2.values = ["List plugins",
                          "Load new plugin", 
                          "Setup File Writing", 
                          "Start Writing", 
                          "Stop Writing",
                          "Exit"]
        self.t2.when_value_edited = self.button

    def button(self):
        selected = self.t2.entry_widget.value
        if selected == 0:
          msg = IpcMessage("cmd", "configure")
          config = {
                     "list": True
                   }
          msg.set_param("plugin", config)
          self.parentApp.send_message(msg)
        if selected == 1:
          self.parentApp.setNextForm("LOAD_PLUGIN")
          self.editing = False
          self.parentApp.switchFormNow()
        if selected == 2:
          self.parentApp.setNextForm("SETUP_FILE")
          self.editing = False
          self.parentApp.switchFormNow()
        if selected == 3:
          msg = IpcMessage("cmd", "configure")
          config = {
                     "write": True
                   }
          msg.set_param("hdf", config)
          self.parentApp.send_message(msg)
        if selected == 4:
          msg = IpcMessage("cmd", "configure")
          config = {
                     "write": False
                   }
          msg.set_param("hdf", config)
          self.parentApp.send_message(msg)
        if selected == 5:
          self.parentApp.setNextForm(None)
          self.parentApp.switchFormNow()
          
    def while_waiting(self):
        if self.parentApp._current_value != self.parentApp._prev_value:
            self.t3.values = self.parentApp._current_value.split("\n")
            self.t3.display()
            self.parentApp._prev_value = self.parentApp._current_value
        self.t2.entry_widget.value = None
        self.t2.entry_widget._old_value = None
        self.t2.display()

class LoadPlugin(npyscreen.ActionForm):
    def create(self):
        self.name = "ODIN File Writer Client"
        self.add(npyscreen.TitleText, labelColor="LABELBOLD", name="Load a new plugin", value="", editable=False)
        self.ctrl1 = self.add(npyscreen.TitleText, name="Plugin library: ", value="/home/gnx91527/work/percival/framereceiver/build/lib/libDummyPlugin.so")
        self.ctrl2 = self.add(npyscreen.TitleText, name="Plugin index:   ", value="dummy")
        self.ctrl3 = self.add(npyscreen.TitleText, name="Plugin name:    ", value="DummyPlugin")

    def on_ok(self):
        msg = IpcMessage("cmd", "configure")
        config = {
                     "load": {
                         "library": self.ctrl1.value,
                         "index": self.ctrl2.value,
                         "name": self.ctrl3.value,
                     }
                 }
        msg.set_param("plugin", config)
        self.parentApp.send_message(msg)

    def afterEditing(self):
        self.parentApp.setNextForm("MAIN_MENU")
        self.editing = False
        self.parentApp.switchFormNow()

class SetupFileWriting(npyscreen.ActionForm):
    def create(self):
        self.name = "ODIN File Writer Client"
        self.add(npyscreen.TitleText, labelColor="LABELBOLD", name="Setup file writing", value="", editable=False)
        self.ctrl1 = self.add(npyscreen.TitleText, name="File name:        ", value="test_0.hdf5")
        self.ctrl2 = self.add(npyscreen.TitleText, name="File path:        ", value="/tmp/")
        self.ctrl3 = self.add(npyscreen.TitleText, name="Number of frames: ", value="3")

    def on_ok(self):
        msg = IpcMessage("cmd", "configure")
        config = {
                   "filename": self.ctrl1.value,
                   "filepath": self.ctrl2.value,
                   "frames": int(self.ctrl3.value),
                 }
        msg.set_param("hdf", config)
        self.parentApp.send_message(msg)

    def afterEditing(self):
        self.parentApp.setNextForm("MAIN_MENU")
        self.editing = False
        self.parentApp.switchFormNow()

def options():
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--control", default="tcp://127.0.0.1:5004", help="Control endpoint")
    args = parser.parse_args()
    return args


def main():
    args = options()

    app = PercivalClientApp(args.control)
    app.run()


if __name__ == '__main__':
    main()

