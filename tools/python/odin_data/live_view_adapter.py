"""ODIN data live view adapter.

This module implements an odin-control adapter capable of rendering odin-data live
view images to users.

Created on 8th October 2018

:author: Adam Neaves, STFC Application Engineering Gruop
"""

import logging
import re
from collections import OrderedDict
import numpy as np
import cv2
from tornado.escape import json_decode

from odin_data.ipc_tornado_channel import IpcTornadoChannel
from odin_data.ipc_channel import IpcChannelException
from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, response_types
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError

ENDPOINTS_CONFIG_NAME = 'live_view_endpoints'
COLORMAP_CONFIG_NAME = 'default_colormap'

DEFAULT_ENDPOINT = 'tcp://127.0.0.1:5020'
DEFAULT_COLORMAP = "Jet"


class LiveViewAdapter(ApiAdapter):
    """Live view adapter class.

    This class implements the live view adapter for odin-control.
    """

    def __init__(self, **kwargs):
        """
        Initialise the adapter.

        Creates a LiveViewer Object that handles the major logic of the adapter.

        :param kwargs: Key Word arguments given from the configuration file,
        which is copied into the options dictionary.
        """
        logging.debug("Live View Adapter init called")
        super(LiveViewAdapter, self).__init__(**kwargs)

        if self.options.get(ENDPOINTS_CONFIG_NAME, False):
            endpoints = [x.strip() for x in self.options.get(ENDPOINTS_CONFIG_NAME, "").split(',')]
        else:
            logging.debug("Setting default endpoint of '%s'", DEFAULT_ENDPOINT)
            endpoints = [DEFAULT_ENDPOINT]

        if self.options.get(COLORMAP_CONFIG_NAME, False):
            default_colormap = self.options.get(COLORMAP_CONFIG_NAME, "")
        else:
            default_colormap = "Jet"

        self.live_viewer = LiveViewer(endpoints, default_colormap)

    @response_types('application/json', 'image/*', default='application/json')
    def get(self, path, request):
        """
        Handle a HTTP GET request from a client, passing this to the Live Viewer object.

        :param path: The path to the resource requested by the GET request
        :param request: Additional request parameters
        :return: The requested resource, or an error message and code if the request was invalid.
        """
        try:
            response, content_type, status = self.live_viewer.get(path, request)
        except ParameterTreeError as param_error:
            response = {'response': 'LiveViewAdapter GET error: {}'.format(param_error)}
            content_type = 'application/json'
            status = 400

        return ApiAdapterResponse(response, content_type=content_type, status_code=status)

    @response_types('application/json', default='application/json')
    def put(self, path, request):
        """
        Handle a HTTP PUT request from a client, passing it to the Live Viewer Object.

        :param path: path to the resource
        :param request: request object containing data to PUT to the resource
        :return: the requested resource after changing, or an error message and code if invalid
        """
        logging.debug("REQUEST: %s", request.body)
        try:
            data = json_decode(request.body)
            self.live_viewer.set(path, data)
            response, content_type, status = self.live_viewer.get(path)

        except ParameterTreeError as param_error:
            response = {'response': 'LiveViewAdapter PUT error: {}'.format(param_error)}
            content_type = 'application/json'
            status = 400

        return ApiAdapterResponse(response, content_type=content_type, status_code=status)

    def cleanup(self):
        """Clean up the adapter on shutdown. Calls the Live View object's cleanup method."""
        self.live_viewer.cleanup()


class LiveViewer(object):
    """
    Live viewer main class.

    This class handles the major logic of the adapter, including generation of the images from data.
    """

    def __init__(self, endpoints, default_colormap):
        """
        Initialise the LiveViewer object.

        This method creates the IPC channel used to receive images from odin-data and
        assigns a callback method that is called when data arrives at the channel.
        It also initialises the Parameter tree used for HTTP GET and SET requests.
        :param endpoints: the endpoint address that the IPC channel subscribes to.
        """
        logging.debug("Initialising LiveViewer")

        self.img_data = np.arange(0, 1024, 1).reshape(32, 32)
        self.clip_min = None
        self.clip_max = None
        self.header = {}
        self.endpoints = endpoints
        self.ipc_channels = []
        for endpoint in self.endpoints:
            try:
                tmp_channel = SubSocket(self, endpoint)
                self.ipc_channels.append(tmp_channel)
                logging.debug("Subscribed to endpoint: %s", tmp_channel.endpoint)
            except IpcChannelException as chan_error:
                logging.warning("Unable to subscribe to %s: %s", endpoint, chan_error)

        logging.debug("Connected to %d endpoints", len(self.ipc_channels))

        if not self.ipc_channels:
            logging.warning(
                "Warning: No subscriptions made. Check the configuration file for valid endpoints")

        # Define a list of available cv2 colormaps
        self.cv2_colormaps = {
            "Autumn": cv2.COLORMAP_AUTUMN,
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

        # Build a sorted list of colormap options mapping readable name to lowercase option
        self.colormap_options = OrderedDict()
        for colormap_name in sorted(self.cv2_colormaps.keys()):
            self.colormap_options[colormap_name.lower()] = colormap_name

        # Set the selected colormap to the default
        if default_colormap.lower() in self.colormap_options:
            self.selected_colormap = default_colormap.lower()
        else:
            self.selected_colormap = "jet"

        self.rendered_image = self.render_image()

        self.param_tree = ParameterTree({
            "name": "Live View Adapter",
            "endpoints": (self.get_channel_endpoints, None),
            "frame": (lambda: self.header, None),
            "colormap_options": self.colormap_options,
            "colormap_selected": (self.get_selected_colormap, self.set_selected_colormap),
            "data_min_max": (lambda: [int(self.img_data.min()), int(self.img_data.max())], None),
            "frame_counts": (self.get_channel_counts, self.set_channel_counts),
            "clip_range": (lambda: [self.clip_min, self.clip_max], self.set_clip)
        })

    def get(self, path, _request=None):
        """
        Handle a HTTP get request.

        Checks if the request is for the image or another resource, and responds accordingly.
        :param path: the URI path to the resource requested
        :param request: Additional request parameters.
        :return: the requested resource,or an error message and code, if the request is invalid.
        """
        path_elems = re.split('[/?#]', path)
        if path_elems[0] == 'image':
            if self.img_data is not None:
                response = self.rendered_image
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
        Handle a HTTP PUT i.e. set request.

        :param path: the URI path to the resource
        :param data: the data to PUT to the resource
        """
        self.param_tree.set(path, data)

    def create_image_from_socket(self, msg):
        """
        Create an image from data received on the socket.

        This callback function is called when data is ready on the IPC channel. It creates
        the image data array from the raw data sent by the Odin Data Plugin, reshaping
        it to a multi dimensional array matching the image dimensions.
        :param msg: a multipart message containing the image header, and raw image data.
        """
        # Message should be a list from multi part message.
        # First part will be the json header from the live view, second part is the raw image data
        header = json_decode(msg[0])

        # json_decode returns dictionary encoded in unicode. Convert to normal strings
        header = self.convert_to_string(header)
        logging.debug("Got image with header: %s", header)

        # create a np array of the image data, of type specified in the frame header
        img_data = np.fromstring(msg[1], dtype=np.dtype(header['dtype']))

        self.img_data = img_data.reshape([int(header["shape"][0]), int(header["shape"][1])])
        self.header = header

        self.rendered_image = self.render_image(
            self.selected_colormap, self.clip_min, self.clip_max)

    def render_image(self, colormap=None, clip_min=None, clip_max=None):
        """
        Render an image from the image data, applying a colormap to the greyscale data.

        :param colormap: Desired image colormap. if None, uses the default colormap.
        :param clip_min: The minimum pixel value desired. If a pixel is lower than this value,
        it is set to this value.
        :param clip_max: The maximum pixel value desired. If a pixel is higher than this value,
        it is set to this value.
        :return: The rendered image binary data, encoded into a string so it can be returned
        by a GET request.
        """
        if colormap is None:
            colormap = self.selected_colormap

        if clip_min is not None and clip_max is not None:
            if clip_min > clip_max:
                clip_min = None
                clip_max = None
                logging.warning("Clip minimum cannot be more than clip maximum")

        if clip_min is not None or clip_max is not None:
            img_clipped = np.clip(self.img_data, clip_min, clip_max)  # clip image

        else:
            img_clipped = self.img_data

        # Scale to 0-255 for colormap
        img_scaled = self.scale_array(img_clipped, 0, 255).astype(dtype=np.uint8)

        # Apply colormap
        cv2_colormap = self.cv2_colormaps[self.colormap_options[colormap]]
        img_colormapped = cv2.applyColorMap(img_scaled, cv2_colormap)

        # Most time consuming step, depending on image size and the type of image
        img_encode = cv2.imencode(
            '.png', img_colormapped, params=[cv2.IMWRITE_PNG_COMPRESSION, 0])[1]
        return img_encode.tostring()

    @staticmethod
    def scale_array(src, tmin, tmax):
        """
        Set the range of image data.

        The ratio between pixels should remain the same, but the total range should be rescaled
        to fit the desired minimum and maximum
        :param src: the source array to rescale
        :param tmin: the target minimum
        :param tmax: the target maximum
        :return: an array of the same dimensions as the source, but with the data rescaled.
        """
        smin, smax = src.min(), src.max()

        downscaled = (src.astype(float) - smin) / (smax - smin)
        rescaled = (downscaled * (tmax - tmin) + tmin).astype(src.dtype)

        return rescaled

    def convert_to_string(self, obj):
        """
        Convert all unicode parts of a dictionary or list to standard strings.

        This method may not handle special characters well!
        :param obj: the dictionary, list, or unicode string
        :return: the same data type as obj, but with unicode strings converted to python strings.
        """
        if isinstance(obj, dict):
            return {self.convert_to_string(key): self.convert_to_string(value)
                    for key, value in obj.items()}
        elif isinstance(obj, list):
            return [self.convert_to_string(element) for element in obj]
        elif isinstance(obj, unicode):
            return obj.encode('utf-8')

        return obj

    def cleanup(self):
        """Close the IPC channels ready for shutdown."""
        for channel in self.ipc_channels:
            channel.cleanup()

    def get_selected_colormap(self):
        """
        Get the default colormap for the adapter.

        :return: the default colormap for the adapter
        """
        return self.selected_colormap

    def set_selected_colormap(self, colormap):
        """
        Set the selected colormap for the adapter.

        :param colormap: colormap to select
        """
        if colormap.lower() in self.colormap_options:
            self.selected_colormap = colormap.lower()

    def set_clip(self, clip_array):
        """
        Set the image clipping, i.e. max and min values to render.

        :param clip_array: array of min and max values to clip
        """
        if (clip_array[0] is None) or isinstance(clip_array[0], int):
            self.clip_min = clip_array[0]
          
        if (clip_array[1] is None) or isinstance(clip_array[1], int):
            self.clip_max = clip_array[1]

    def get_channel_endpoints(self):
        """
        Get the list of endpoints this adapter is subscribed to.

        :return: a list of endpoints
        """
        endpoints = []
        for channel in self.ipc_channels:
            endpoints.append(channel.endpoint)

        return endpoints

    def get_channel_counts(self):
        """
        Get a dict of the endpoints and the count of how many frames came from that endpoint.

        :return: A dict, with the endpoint as a key, and the number of images from that endpoint
        as the value
        """
        counts = {}
        for channel in self.ipc_channels:
            counts[channel.endpoint] = channel.frame_count

        return counts

    def set_channel_counts(self, data):
        """
        Set the channel frame counts.

        This method is used to reset the channel frame counts to known values.
        :param data: channel frame count data to set
        """
        data = self.convert_to_string(data)
        logging.debug("Data Type: %s", type(data).__name__)
        for channel in self.ipc_channels:
            if channel.endpoint in data:
                logging.debug("Endpoint %s in request", channel.endpoint)
                channel.frame_count = data[channel.endpoint]


class SubSocket(object):
    """
    Subscriber Socket class.

    This class implements an IPC channel subcriber socker and sets up a callback function
    for receiving data from that socket that counts how many images it receives during its lifetime.
    """

    def __init__(self, parent, endpoint):
        """
        Initialise IPC channel as a subscriber, and register the callback.

        :param parent: the class that created this object, a LiveViewer, given so that this object
        can reference the method in the parent
        :param endpoint: the URI address of the socket to subscribe to
        """
        self.parent = parent
        self.endpoint = endpoint
        self.frame_count = 0
        self.channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_SUB, endpoint=endpoint)
        self.channel.subscribe()
        self.channel.connect()
        # register the get_image method to be called when the ZMQ socket receives a message
        self.channel.register_callback(self.callback)

    def callback(self, msg):
        """
        Handle incoming data on the socket.

        This callback method is called whenever data arrives on the IPC channel socket.
        Increments the counter, then passes the message on to the image renderer of the parent.
        :param msg: the multipart message from the IPC channel
        """
        self.frame_count += 1
        self.parent.create_image_from_socket(msg)

    def cleanup(self):
        """Cleanup channel when the server is closed. Closes the IPC channel socket correctly."""
        self.channel.close()
