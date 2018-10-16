"""
Created on 8th October 2018

:author: Adam Neaves
"""

import logging
from tornado.escape import json_decode
from odin_data.ipc_tornado_channel import IpcTornadoChannel

import numpy as np
import cv2
import re
from collections import OrderedDict

from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, response_types
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError

ENDPOINTS_CONFIG_NAME = 'live_view_endpoints'
COLORMAP_CONFIG_NAME = 'default_colormap'

DEFAULT_ENDPOINT = 'tcp://127.0.0.1:5020'
DEFAULT_COLORMAP = "Jet"


class LiveViewAdapter(ApiAdapter):

    def __init__(self, **kwargs):
        """
        Initialise the adapter. Creates a LiveViewer Object that handles the major logic of the adapter.

        :param kwargs: Key Word arguments given from the configuration file,
        which is copied into the options dictionary.
        """
        logging.debug("Live View Adapter init called")
        super(LiveViewAdapter, self).__init__(**kwargs)

        if self.options.get(ENDPOINTS_CONFIG_NAME, False):
            endpoints = [x.strip() for x in self.options.get(ENDPOINTS_CONFIG_NAME, "").split(',')]
        else:
            logging.debug("Setting default endpoint of '{}'".format(DEFAULT_ENDPOINT))
            endpoints = [DEFAULT_ENDPOINT]

        if self.options.get(COLORMAP_CONFIG_NAME, False):
            default_colormap = self.options.get(COLORMAP_CONFIG_NAME, "")
        else:
            default_colormap = "Jet"

        self.live_viewer = LiveViewer(endpoints, default_colormap)

    @response_types('application/json', 'image/*', default='application/json')
    def get(self, path, request):
        """
        Handles a HTTP GET request from a client, passing this to the Live Viewer object

        :param path: The path to the resource requested by the GET request
        :param request: Additional request parameters
        :return: The requested resource, or an error message and code if the request was invalid.
        """
        try:
            response, content_type, status = self.live_viewer.get(path, request)
        except ParameterTreeError as e:
            response = {'response': 'LiveViewAdapter GET error: {}'.format(e.message)}
            content_type = 'application/json'
            status = 400

        return ApiAdapterResponse(response, content_type=content_type, status_code=status)

    @response_types('application/json', default='application/json')
    def put(self, path, request):
        """
        Handles a HTTP PUT request from a client, passing it to the Live Viewer Object
        :param path: path to the resource
        :param request: request object containing data to PUT to the resource
        :return: the requested resource after changing, or an error message and code if it was invalid.
        """
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
        """
        Clean up the adapter on shutdown. Calls the Live View object's cleanup method.
        """
        self.live_viewer.cleanup()


class LiveViewer(object):
    """
    Live View object, which handles the major logic of the adapter, including the compliation of the images from data.
    """
    def __init__(self, endpoints, default_colormap):
        """
        Initialise the Live View object, creating the IPC channel used to receive images from Odin Data,
        and assigning a callback method that is called when data arrives at the channel.
        It also initialises the Parameter tree used for HTTP GET and SET requests.
        :param endpoints: the endpoint address that the IPC channel subscribes to.
        """
        logging.debug("Initialising LiveViewer")

        self.img_data = np.arange(0, 1024, 1).reshape(32, 32)
        self.header = {}
        self.endpoints = endpoints
        self.ipc_channels = []
        for endpoint in self.endpoints:
            try:
                tmp_channel = SubSocket(self, endpoint)
                self.ipc_channels.append(tmp_channel)
                logging.debug("Subscribed to endpoint: {}".format(tmp_channel.endpoint))
            except Exception as e:
                logging.warning("Unable to subscribe to {0}: {1}".format(endpoint, e))

        logging.debug("Connected to {} endpoints".format(len(self.ipc_channels)))
        if len(self.ipc_channels) == 0:
            logging.warning("Warning: No subscriptions made. Check the configuration file for valid endpoints")

        self.colormap_options = {"autumn": cv2.COLORMAP_AUTUMN,
                                 "bone": cv2.COLORMAP_BONE,
                                 "jet": cv2.COLORMAP_JET,
                                 "winter": cv2.COLORMAP_WINTER,
                                 "rainbow": cv2.COLORMAP_RAINBOW,
                                 "ocean": cv2.COLORMAP_OCEAN,
                                 "summer": cv2.COLORMAP_SUMMER,
                                 "spring": cv2.COLORMAP_SPRING,
                                 "cool": cv2.COLORMAP_COOL,
                                 "hsv": cv2.COLORMAP_HSV,
                                 "pink": cv2.COLORMAP_PINK,
                                 "hot": cv2.COLORMAP_HOT,
                                 "parula": cv2.COLORMAP_PARULA
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

        if default_colormap.lower() in self.colormap_options:
            self.selected_colormap = self.colormap_options[default_colormap.lower()]
        else:
            self.selected_colormap = self.colormap_options["jet"]

        self.param_tree = ParameterTree(
            {"name": "Live View Adapter",
             "endpoints": (lambda: self.get_channel_endpoints(), None),
             "frame": (lambda: self.header, None),
             "colormap_options": (lambda: self.get_colormap_options_list(), None),
             "colormap_default": (lambda: self.get_default_colormap(), None),
             "data_min_max": (lambda: [int(self.img_data.min()), int(self.img_data.max())], None),
             "frame_counts": (lambda: self.get_channel_counts(), None)
             }
        )

    def get(self, path, request=None):
        """
        Handles a HTTP get request. Checks if the request is for the image or another resource, and responds accordingly
        :param path: the URI path to the resource requested
        :param request: Additional request parameters. Used for the image requests
        :return: the requested resource, or an error message and code, depending on the validity of the request.
        """
        path_elems = re.split('[/?#]', path)
        if path_elems[0] == 'image':
            if self.img_data is not None:
                colormap = None
                clip_max = None
                clip_min = None
                if request is not None:
                    # Get request parameters needed for the image rendering
                    if "colormap" in request.arguments:
                        colormap_name = request.arguments["colormap"][0].lower()
                        colormap = self.colormap_options[colormap_name]
                    if "clip-max" in request.arguments:
                        clip_max = int(request.arguments["clip-max"][0])
                    if "clip-min" in request.arguments:
                        clip_min = int(request.arguments["clip-min"][0])

                response = self.render_image(colormap, clip_min, clip_max)
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
        """
        Handles a HTTP set request
        :param path: the URI path to the resource
        :param data: the data to PUT to the resource
        """
        self.param_tree.set(path, data)

    def create_image_from_socket(self, msg):
        """
        Callback function, called when data is ready on the IPC channel.
        Creates the image data array from the raw data sent by the Odin Data Plugin, reshaping it to a
        multi dimensional array matching the image dimensions.
        :param msg: a multipart message containing the image header, and raw image data.
        :param socket: the socket the message came from
        """
        # message should be a list from multi part message. first part will be the json header from the live view
        # second part is the raw image data
        logging.debug("Creating Image from message")
        header = json_decode(msg[0])
        # json_decode returns dictionary encoded in unicode. Convert to normal strings
        header = self.convert_to_string(header)
        logging.debug(header)
        # create a np array of the image data, of type specified in the frame header
        img_data = np.fromstring(msg[1], dtype=np.dtype(header['dtype']))

        self.img_data = img_data.reshape([int(header["shape"][0]), int(header["shape"][1])])
        self.header = header

    def render_image(self, colormap, clip_min, clip_max):
        """
        Render an image from the image data, applying a colormap to the greyscale data.
        :param colormap: Desired image colormap. if None, uses the default colormap.
        :param clip_min: The minimum pixel value desired. If a pixel is lower than this value, it is set to this value.
        :param clip_max: The maximum pixel value desired. If a pixel is higher than this value, it is set to this value.
        :return: The rendered image binary data, encoded into a string so it can be returned by a GET request.
        """
        if colormap is None:
            colormap = self.selected_colormap
        if clip_min is not None and clip_max is not None:
            if clip_min >= clip_max:
                clip_min = None
                clip_max = None
                logging.warning("ERROR: Clip Min cannot be equal to or more than clip_max")
        if clip_min is not None or clip_max is not None:
            img_clipped = np.clip(self.img_data, clip_min, clip_max)  # clip image

        else:
            img_clipped = self.img_data
        img_scaled = self.scale_array(img_clipped, 0, 255).astype(dtype=np.uint8)  # scale to 0-255 for colormap
        img_colormapped = cv2.applyColorMap(img_scaled, colormap)
        # most time consuming step. Depending on image size and the type of image
        img_encode = cv2.imencode('.png', img_colormapped, params=[cv2.IMWRITE_PNG_COMPRESSION, 0])[1]
        return img_encode.tostring()

    @staticmethod
    def scale_array(src, tmin, tmax):
        """
        Set the range of image data. The ratio between pixels should remain the same,
        but the total range should be rescaled to fit the desired minimum and maximum
        :param src: the source array to rescale
        :param tmin: the target minimum
        :param tmax: the target maximum
        :return: an array of the same dimensions as the source, but with the data rescaled.
        """
        smin, smax = src.min(), src.max()
        return ((tmax - tmin) * (src - smin)) / (smax - smin) + tmin

    # this method copied from Stack Overflow, for converting unicode strings to str objects in a dict
    def convert_to_string(self, obj):
        """
        Converts all unicode parts of a dictionary or list to standard strings.
        This method may not handle special characters well!
        :param obj: the dictionary, list, or unicode string
        :return: the same data type as obj, but with all unicode strings converted to python strings.
        """
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
        """
        Closes the IPC channels ready for shutdown.
        """
        for channel in self.ipc_channels:
            channel.cleanup()

    def get_colormap_options_list(self):
        """
        Gets an ordered list of colormap options from the colormap dictionary
        :return: an ordered list of colormap options.
        """
        options_dict = OrderedDict(sorted(self.colormap_keys_capitalisation.items()))
        return options_dict

    def get_default_colormap(self):
        """
        Gets the default colormap for the adapter
        :return: the default colormap for the adapter
        """
        for name, value in self.colormap_options.items():
            if self.selected_colormap == value:
                return name.lower()

    def get_channel_endpoints(self):
        """
        Get the list of endpoints this adapter is subscribed to
        :return: a list of endpoints
        """
        endpoints = []
        for channel in self.ipc_channels:
            endpoints.append(channel.endpoint)

        return endpoints

    def get_channel_counts(self):
        """
        Get a dict of the endpoints and the count of how many frames came from that endpoint
        :return: A dict, with the endpoint as a key, and the number of images from that endpoint as the value
        """
        counts = {}
        for channel in self.ipc_channels:
            counts[channel.endpoint] = channel.frame_count

        return counts


class SubSocket(object):
    """
    The Subscriber Socket object, this initialises the IPC Channel and sets up a callback function for receiving data
    from that socket that counts how many images it receives during its lifetime.
    """
    def __init__(self, parent, endpoint):
        """
        Initializer, sets up the IPC channel as a subscriber, and registers the callback
        :param parent: the class that created this object, a LiveViewer, given so that this object can reference the
        method in the parent
        :param endpoint: the URI address of the socket to subscribe to
        """
        self.parent = parent
        self.endpoint = endpoint
        self.frame_count = 0
        self.channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_SUB, endpoint=endpoint)
        self.channel.subscribe()
        self.channel.connect()
        # register the get_image method to automatically be called when the ZMQ socket receives a message
        self.channel.register_callback(self.callback)

    def callback(self, msg):
        """
        The callback method that gets called whenever data arrives on the IPC channel socket. Increments the counter,
        then passes the message on to the image renderer of the parent.
        :param msg: the multipart message from the IPC channel
        """
        self.frame_count += 1
        self.parent.create_image_from_socket(msg)

    def cleanup(self):
        """
        Cleanup method for when the server is closed. Closes the IPC channel socket correctly.
        """
        self.channel.close()
