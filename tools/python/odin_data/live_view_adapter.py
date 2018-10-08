"""
Created on 8th October 2018

:author: Adam Neaves
"""

import logging
from concurrent import futures
import time
from tornado.ioloop import IOLoop
from tornado.concurrent import run_on_executor
from odin_data.ipc_tornado_channel import IpcTornadoChannel
import json
import numpy as np

from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, request_types, response_types


class LiveViewAdapter(ApiAdapter):

    def __init__(self, **kwargs):

        logging.debug("Live View Adapter init called")
        super(LiveViewAdapter, self).__init__(**kwargs)

        if self.options.get('live_view_endpoint', False):
            self.endpoint = self.options.get('live_view_endpoint', "")
        else:
            logging.debug("Setting default endpoint of 'tcp://127.0.0.1:5020'")
            self.endpoint = "tcp://127.0.0.1:5020"

        self.ipc_channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_SUB, endpoint=self.endpoint)
        self.ipc_channel.subscribe()
        self.ipc_channel.connect()
        # register the get_image method to automatically be called when the ZMQ socket receives a message
        self.ipc_channel.register_callback(self.get_image)

    def get_image(self, msg):
        # message should be a list from multi part message. first part will be the json header from the live view
        # second part is the raw image data
        logging.debug("Callback 'Get Image' Called")
        header = json.load(msg[0])
        img_raw = msg[1]
        img_buf = memoryview(img_raw)
        img_data = np.frombuffer(img_buf, dtype=header['dtype'])
        img_data = np.reshape([int(header["shape"][0]), int(header["shape"][1])])
