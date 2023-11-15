"""
Created on 6th September 2017

:author: Alan Greer
"""
import logging
import os
import threading
import queue
from tornado.escape import json_decode

from odin.adapters.adapter import (
    ApiAdapterRequest,
    ApiAdapterResponse,
    request_types,
    response_types,
    wants_metadata
)
from tornado import escape

from odin_data.control.odin_data_adapter import OdinDataAdapter
from odin.adapters.parameter_tree import ParameterAccessor, ParameterTree

FP_ADAPTER_KEY = 'fr_adapter_name'
FP_DEFAULT_ADAPTER_KEY = 'fr'
PCT_BUFFER_FREE_KEY = 'buffer_threshold'

def bool_from_string(value):
    bool_value = False
    if value.lower() == 'true' or value.lower() == '1':
        bool_value = True
    return bool_value


class FrameProcessorAdapter(OdinDataAdapter):
    """
    OdinDataAdapter class

    This class provides the adapter interface between the ODIN server and the ODIN-DATA detector system,
    transforming the REST-like API HTTP verbs into the appropriate frameProcessor ZeroMQ control messages
    """
    VERSION_CHECK_CONFIG_ITEMS = ['plugin']

    def __init__(self, **kwargs):
        """
        Initialise the OdinDataAdapter object

        :param kwargs:
        """
        logging.debug("FPA init called")
        super(FrameProcessorAdapter, self).__init__(**kwargs)

        self._fr_adapter_name = FP_DEFAULT_ADAPTER_KEY
        if FP_ADAPTER_KEY in kwargs:
            self._fr_adapter_name = kwargs[FP_ADAPTER_KEY]

        self._fr_adapter = None

        # Set the bufer threshold check, default to 0.0 (no check) if the option
        # does not exist or is badly formatted
        self._fr_pct_buffer_threshold = 0.0
        try:
            self._fr_pct_buffer_threshold = float(self.options.get(PCT_BUFFER_FREE_KEY, 0.0))
        except ValueError:
            logging.error("Could not set the buffer threshold to: {}".format(kwargs[PCT_BUFFER_FREE_KEY]))

        # Create the command handling thread
        self._command_lock = threading.Lock()
        self._command_queue = queue.Queue()
        self._command_thread = threading.Thread(target=self.command_loop)
        self._command_thread.start()

        self.setup_rank()

        self._acquisition_id = ''
        self._frames = 0
        self._file_path = ''
        self._file_prefix = ''
        self._file_extension = 'h5'
        self._fp_params = ParameterTree({
            'config': {
                'hdf': {
                    'acquisition_id': (self._get('_acquisition_id'), lambda v: self._set('_acquisition_id', v), {}),
                    'frames': (self._get('_frames'), lambda v: self._set('_frames', v), {}),
                    'file': {
                        'path': (self._get('_file_path'), lambda v: self._set('_file_path', v), {}),
                        'prefix': (self._get('_file_prefix'), lambda v: self._set('_file_prefix', v), {}),
                        'extension': (self._get('_file_extension'), lambda v: self._set('_file_extension', v), {})
                    },
                    'write': (lambda: 1, self.queue_write, {})
                }
            }
        }, mutable=True)

    def _set(self, attr, val):
        logging.debug("_set called: {}  {}".format(attr, val))
        setattr(self, attr, val)

    def _get(self, attr):

        return lambda : getattr(self, attr)

    def cleanup(self):
        self._command_queue.put((None, None), block=False)

    def queue_write(self, value):
        logging.debug("Queueing write command: {}".format(value))
        self._command_queue.put((self.execute_write, value), block=False)

    def command_loop(self):
        """ This method runs in a different thread to the main IOLoop
        and can therefore be used to execute long running sequences
        of commands and where response checking is required.

        Each queued command is a tuple, first element the method to call
        and the second element the argument.
        """
        running = True
        while running:
            try:
                (command, arg) = self._command_queue.get()
                if command:
                    with self._command_lock:
                        command(arg)
                else:
                    running = False
            except Exception as e:
                type_, value_, traceback_ = sys.exc_info()
                ex = traceback.format_exception(type_, value_, traceback_)
                logging.error(e)
                self.set_error("Unhandled exception: {} => {}".format(str(e), str(ex)))

    def initialize(self, adapters):
        """Initialize the adapter after it has been loaded.
        Find and record the FR adapter for later error checks
        """
        if self._fr_adapter_name in adapters:
            self._fr_adapter = adapters[self._fr_adapter_name]
            logging.info("FP adapter initiated connection to FR adapter: {}".format(self._fr_adapter_name))
        else:
            logging.error("FP adapter could not connect to the FR adapter: {}".format(self._fr_adapter_name))

    @request_types('application/json', 'application/vnd.odin-native')
    @response_types('application/json', default='application/json')
    def get(self, path, request):

        """
        Implementation of the HTTP GET verb for OdinDataAdapter

        :param path: URI path of the GET request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        status_code = 200
        response = {}

        try:
            response = self._fp_params.get(path, wants_metadata(request))
        except:
            return super(FrameProcessorAdapter, self).get(path, request)

        return ApiAdapterResponse(response, status_code=status_code)

    @request_types('application/json', 'application/vnd.odin-native')
    @response_types('application/json', default='application/json')
    def put(self, path, request):  # pylint: disable=W0613

        """
        Implementation of the HTTP PUT verb for OdinDataAdapter

        :param path: URI path of the PUT request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        status_code = 200
        response = {}
        logging.debug("PUT path: %s", path)
        logging.debug("PUT request: %s", escape.url_unescape(request.body))

        try:
            self._fp_params.set(path, json_decode(request.body))
        except Exception as e:
            return super(FrameProcessorAdapter, self).put(path, request)

        return ApiAdapterResponse(response, status_code=status_code)

    def execute_write(self, write):
        try:
            config = {'hdf': {'write': write}}
            logging.debug("Executing write command {}".format(config))
            if write:
                # Before attempting to write files, make some simple error checks

                # Check if we have a valid buffer status from the FR adapter
                valid, reason = self.check_fr_status()
                if not valid:
                    raise RuntimeError(reason)

                # First setup the rank for the frameProcessor applications
                self.setup_rank()
                rejected = False
                timed_out = False
                for client in self._clients:
                    # Send the configuration required to setup the acquisition
                    # The file path is the same for all clients
                    parameters = {
                        'hdf': {
                            'frames': self._frames
                        }
                    }
                    # Send the number of frames first
                    msg = client.send_configuration(parameters)
                    if client.wait_for_response(msg.get_msg_id()):
                        timed_out = True
                    if client.check_for_rejection(msg.get_msg_id()):
                        rejected = True

                if timed_out:
                    raise RuntimeError("Setting number of frames timed out")
                if rejected:
                    raise RuntimeError("Setting number of frames was rejected")

                rejected = False
                timed_out = False
                for client in self._clients:
                    parameters = {
                        'hdf': {
                            'acquisition_id': self._acquisition_id,
                            'file': {
                                'path': self._file_path,
                                'name': self._file_prefix,
                                'extension': self._file_extension
                            }
                        }
                    }
                    msg = client.send_configuration(parameters)
                    if client.wait_for_response(msg.get_msg_id()):
                        timed_out = True
                    if client.check_for_rejection(msg.get_msg_id()):
                        rejected = True

                if timed_out:
                    raise RuntimeError("Setting write parameters timed out")
                if rejected:
                    raise RuntimeError("Setting write parameters was rejected")

            rejected = False
            timed_out = False
            for client in self._clients:
                # Send the configuration required to start the acquisition
                msg = client.send_configuration(config)
                if client.wait_for_response(msg.get_msg_id()):
                    timed_out = True
                if client.check_for_rejection(msg.get_msg_id()):
                    rejected = True

            if timed_out:
                raise RuntimeError("Setting write parameters timed out")
            if rejected:
                raise RuntimeError("Setting write parameters was rejected")

        except Exception as ex:
            logging.error("Error: %s", ex)
            self.set_error(str(ex))

    def check_fr_status(self):
        valid_check = True
        reason = ''
        # We should have a valid connection to the FR adapter
        if self._fr_adapter is not None:
            # Create an inter adapter request
            req = ApiAdapterRequest(None, accept='application/json')
            status_dict = self._fr_adapter.get(path='status', request=req).data
            if 'value' in status_dict:
                frs = status_dict['value']
                for fr in frs:
                    try:
                        frames_dropped = fr['frames']['dropped']
                        empty_buffers = fr['buffers']['empty']
                        total_buffers = fr['buffers']['total']
                        if frames_dropped > 0:
                            valid_check = False
                            reason = "Frames dropped [{}] on at least one FR".format(frames_dropped)
                        pct_free = 0.0
                        if total_buffers > 0:
                            pct_free = float(empty_buffers) / float(total_buffers) * 100.0
                        if pct_free < self._fr_pct_buffer_threshold:
                            valid_check = False
                            reason = "There are only {}% free buffers left on at least one FR".format(pct_free)
                    except Exception as ex:
                        valid_check = False
                        reason = "Could not complete FR validity check, exception was thrown: {}".format(str(ex))
            else:
                valid_check = False
                reason = "No status returned from the FR adapter"
        else:
            valid_check = False
            reason = "No FR adapter has been registered with the FP adapter"

        return valid_check, reason

    def require_version_check(self, param):
        # If the parameter is in the version check list then request a version update
        if param in self.VERSION_CHECK_CONFIG_ITEMS:
            return True
        return False

    def setup_rank(self):
        # Attempt initialisation of the connected clients
        processes = len(self._clients)
        rank = 0
        for client in self._clients:
            try:
                # Setup the number of processes and the rank for each client
                parameters = {
                    'hdf': {
                        'process': {
                            'number': processes,
                            'rank': rank
                        }
                    }
                }
                client.send_configuration(parameters)
            except Exception as err:
                logging.debug(OdinDataAdapter.ERROR_FAILED_TO_SEND)
                logging.error("Error: %s", err)
            rank += 1
