"""
Created on 8th October 2018

:author: Adam Neaves
"""

import logging
from tornado.escape import json_decode
from odin_data.ipc_tornado_channel import IpcTornadoChannel
from odin_data.ipc_channel import IpcChannelException

import numpy as np
import cv2
import re
from collections import OrderedDict

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

        self.live_viewer = LiveViewer(self.endpoint)

    @response_types('application/json', 'image/*', default='application/json')
    def get(self, path, request):
        try:
            response, content_type, status = self.live_viewer.get(path, request)
        except ParameterTreeError as e:
            response = {'response': 'LiveViewAdapter GET error: {}'.format(e.message)}
            content_type = 'application/json'
            status = 400

        return ApiAdapterResponse(response, content_type=content_type, status_code=status)

    @response_types('application/json', default='application/json')
    def put(self, path, request):

        logging.debug("REQUEST: {}".format(request.body))
        try:
            data = json_decode(request.body)
            self.live_viewer.set(path, data)
            response, content_type, status = self.live_viewer.get(path)

        except ParameterTreeError as e:
            response = {'response': 'LiveViewAdapter PUT error: {}'.format(e.message)}
            content_type = 'application/json'
            status = 400

        return ApiAdapterResponse(response, content_type=content_type, status_code=status)

    def cleanup(self):
        self.live_viewer.cleanup()


class LiveViewer(object):

    def __init__(self, endpoint):
        logging.debug("Initialising LiveViewer")

        self.img_data = None
        self.frame = Frame({})
        self.endpoint = endpoint
        self.ipc_channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_SUB, endpoint=self.endpoint)
        self.ipc_channel.subscribe()
        self.ipc_channel.connect()
        # register the get_image method to automatically be called when the ZMQ socket receives a message
        self.ipc_channel.register_callback(self.create_image_from_socket)

        self.colormap_options = {"Autumn": cv2.COLORMAP_AUTUMN,
                                 "Bone": cv2.COLORMAP_BONE,
                                 "Jet": cv2.COLORMAP_JET,
                                 "Winter": cv2.COLORMAP_WINTER,
                                 "Rainbow": cv2.COLORMAP_RAINBOW,
                                 "Ocean": cv2.COLORMAP_OCEAN,
                                 "Summer": cv2.COLORMAP_SUMMER,
                                 "Spring": cv2.COLORMAP_SPRING,
                                 "Cool": cv2.COLORMAP_COOL,
                                 "HSV": cv2.COLORMAP_HSV,
                                 "Pink": cv2.COLORMAP_PINK,
                                 "Hot": cv2.COLORMAP_HOT,
                                 "Parula": cv2.COLORMAP_PARULA
                                 }
        self.colormap_keys_capitalisation = {"autumn": "Autumn",
                                             "bone": "Bone",
                                             "jet": "Jet",
                                             "winter": "Winter",
                                             "rainbow": "Rainbow",
                                             "ocean": "Ocean",
                                             "summer": "Summer",
                                             "spring": "Spring",
                                             "cool": "Cool",
                                             "hsv": "HSV",
                                             "pink": "Pink",
                                             "hot": "Hot",
                                             "parula": "Parula"
                                             }

        self.selected_colormap = self.colormap_options["Jet"]

        self.param_tree = ParameterTree(
            {"name": "Live View Adapter",
             "endpoint": (lambda: self.endpoint, self.set_endpoint),
             "frame": (lambda: self.frame.header, None),
             "colormap_options": (lambda: self.get_colormap_options_list(), None),
             "colormap_default": (lambda: self.get_default_colormap(), None)
             }
        )

    def get(self, path, request):
        path_elems = re.split('[/?#]', path)
        if path_elems[0] == 'image':
            if self.img_data is not None:
                if "colormap" in request.arguments:
                    colormap_name = request.arguments["colormap"][0]
                    colormap = self.colormap_options[self.colormap_keys_capitalisation[colormap_name]]
                else:
                    colormap = None
                response = self.render_image(colormap=colormap)
                content_type = 'image/png'
                status = 200
            else:
                response = {"response": "LiveViewAdapter: No Image Available"}
                content_type = 'application/json'
                status = 400
        else:

            response = self.param_tree.get(path)
            content_type = 'application/json'
            status = 200

        return response, content_type, status

    def set(self, path, data):
        self.param_tree.set(path, data)

    def create_image_from_socket(self, msg):
        # message should be a list from multi part message. first part will be the json header from the live view
        # second part is the raw image data
        logging.debug("Creating Image from message")
        header = json_decode(msg[0])
        # json_decode returns dictionary encoded in unicode. Convert to normal strings
        header = self.convert_to_string(header)
        logging.debug(header)
        img_data = np.fromstring(msg[1], dtype=np.dtype(header['dtype']))
        img_data = img_data.reshape([int(header["shape"][0]), int(header["shape"][1])])
        self.img_data = cv2.normalize(img_data, dst=None, alpha=0, beta=255, norm_type=cv2.NORM_MINMAX, dtype=cv2.CV_8UC1)
        self.frame.header = header

    def render_image(self, img_data=None, colormap=None):
        # logging.debug("Rendering Image")
        if img_data is not None:
            self.img_data = img_data
        if colormap is None:
            colormap = self.selected_colormap
        # else:
        #     logging.debug("Colormap: " + str(colormap))
        img_scaled = cv2.applyColorMap(self.img_data, colormap)
        # most time consuming step. Depending on image size and the type of image
        img_encode = cv2.imencode('.png', img_scaled, params=[cv2.IMWRITE_PNG_COMPRESSION, 0])[1]
        return img_encode.tostring()

    # this method copied from Stack Overflow, for converting unicode strings to str objects in a dict
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

    def cleanup(self):
        self.ipc_channel.close()

    def get_colormap_options_list(self):
        options_dict = OrderedDict(sorted(self.colormap_keys_capitalisation.items()))
        return options_dict

    def get_default_colormap(self):
        for name, value in self.colormap_options.items():
            if self.selected_colormap == value:
                return name.lower()

    def set_endpoint(self, endpoint):
        try:
            self.endpoint = endpoint
            if self.ipc_channel.endpoint is not self.endpoint:
                self.ipc_channel.connect(self.endpoint)
        except IpcChannelException as e:
            logging.error("IPC Channel Exception: {}".format(e.message))


class Frame(object):

    def __init__(self, header):
        self.header = header

    def get(self, path):
        if path in self.header:
            response = self.header[path]
        else:
            response = None
        return response

