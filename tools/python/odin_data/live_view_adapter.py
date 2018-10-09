"""
Created on 8th October 2018

:author: Adam Neaves
"""

import logging
from concurrent import futures
import time
from tornado.escape import json_decode
from tornado.ioloop import IOLoop
from tornado.concurrent import run_on_executor
from odin_data.ipc_tornado_channel import IpcTornadoChannel

import numpy as np
import cv2
import re

from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, request_types, response_types
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError


class LiveViewAdapter(ApiAdapter):

    def __init__(self, **kwargs):

        logging.debug("Live View Adapter init called")
        super(LiveViewAdapter, self).__init__(**kwargs)

        if self.options.get('live_view_endpoint', False):
            self.endpoint = self.options.get('live_view_endpoint', "")
            logging.debug(self.endpoint)
        else:
            logging.debug("Setting default endpoint of 'tcp://127.0.0.1:5020'")
            self.endpoint = "tcp://127.0.0.1:5020"

        self.ipc_channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_SUB, endpoint=self.endpoint)
        self.ipc_channel.subscribe()
        self.ipc_channel.connect()
        # register the get_image method to automatically be called when the ZMQ socket receives a message
        self.ipc_channel.register_callback(self.create_image_from_socket)

        self.img = None
        self.frame = None

    def create_image_from_socket(self, msg):
        # message should be a list from multi part message. first part will be the json header from the live view
        # second part is the raw image data
        logging.debug("Creating Image from message")
        logging.debug("Number of messages: {}".format(len(msg)))
        header = json_decode(msg[0])
        # json_decode returns dictionary encoded in unicode. Convert to normal strings
        header = self.convert_to_string(header)
        logging.debug(header)
        img_data = np.fromstring(msg[1], dtype=np.dtype(header['dtype']))
        img_data = img_data.reshape([int(header["shape"][0]), int(header["shape"][1])])
        img_scaled = cv2.normalize(img_data, dst=None, alpha=0, beta=65535, norm_type=cv2.NORM_MINMAX)
        self.img = cv2.imencode('.png', img_scaled)[1].tostring()

        self.frame = Frame(header)

    @response_types('application/json', 'image/*', default='application/json')
    def get(self, path, request):

        path_elems = re.split('[/?#]', path)
        if path_elems[0] == 'image':
            # sub_path = '/'.join(path_elems)
            if self.img is not None:
                response = self.img
                content_type = 'image/png'
                status = 200
            else:
                response = {"response": "LiveViewAdapter: No Image Available"}
                content_type = 'application/json'
                status = 400
        else:
            if path in self.options:
                response = self.options.get(path)
            elif self.frame is not None and path in self.frame.header:
                response = self.frame.get(path)
            else:
                response = {'response': 'LiveViewAdapter: GET on path {}'.format(path)}

            content_type = 'application/json'
            status = 200

        return ApiAdapterResponse(response, content_type=content_type, status_code=status)

    @response_types('application/json', default='application/json')
    def put(self, path, request):

        try:
            data = json_decode(request.body)
            if path in self.options:
                self.options[path] = data
                response = {'response': 'LiveViewAdapter PUT called: {} set to {}'.format(path, data)}
            else:
                response = {'response': 'LiveViewAdapter: PUT on path {}'.format(path)}

            content_type = 'application/json'
            status = 200
        except (TypeError, ValueError) as e:
            response = {'response': 'LiveViewAdapter PUT error: {}'.format(e.message)}
            content_type = 'application/json'
            status = 400

        return ApiAdapterResponse(response, content_type=content_type, status_code=status)

    def cleanup(self):
        self.ipc_channel.close()

    def convert_to_string(self, obj):
        if isinstance(obj, dict):
            return {self.convert_to_string(key): self.convert_to_string(value)
                    for key, value in obj.iteritems()}
        elif isinstance(obj, list):
            return [self.convert_to_string(element) for element in obj]
        elif isinstance(obj, unicode):
            return obj.encode('utf-8')
        else:
            return obj


class Frame(object):

    def __init__(self, header):
        self.header = header

    def get(self, path):
        if path in self.header:
            response = self.header[path]
        else:
            response = None
        return response
