"""
Created on 8th July 2019

:author: Alan Greer
"""
import json
import logging
import os
from odin_data.ipc_tornado_client import IpcTornadoClient
from odin_data.util import remove_prefix, remove_suffix
from odin_data.odin_data_adapter import OdinDataAdapter
from odin_data.frame_processor_adapter import FrameProcessorAdapter
from odin.adapters.adapter import ApiAdapter, ApiAdapterRequest, ApiAdapterResponse, request_types, response_types
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError
from tornado import escape
from tornado.escape import json_encode, json_decode
#from tornado.ioloop import IOLoop


class FPCompressionAdapter(FrameProcessorAdapter):
    """
    FPCompressionAdapter class

    This class provides simple control of FP applications that contain a Blosc and HDF5 writer plugin.
    This adapter understands the duplicate commands requried to setup both plugins and exposes compression
    as a simple on/off switch, with other settings moved out to init time configuration settings.
    
    The init time configuration options are described below:
    
    hdf_plugin (hdf) - Name of the HDF5 plugin loaded into the FP applications
    blosc_plugin (blosc) - Name of the Blosc plugin loaded into the FP applications
    compressor (blosclz4) - Name of Blosc compressor to use
    threads (8) - Number of compression threads
    level (4) - Compression level
    shuffle (SHUFFLE) - Level of shuffle to use (none, bit, byte)
        
    """
    PARAM_HDF_PLUGIN = 'hdf_plugin'
    PARAM_BLOSC_PLUGIN = 'blosc_plugin'
    PARAM_COMPRESSOR = 'compressor'
    PARAM_THREADS = 'threads'
    PARAM_LEVEL = 'level'
    PARAM_SHUFFLE = 'shuffle'

    HDF_NO_COMPRESSION = 0
    HDF_BLOSC_COMPRESSION = 3

    DEFAULT_HDF_PLUGIN = 'hdf'
    DEFAULT_BLOSC_PLUGIN = 'blosc'
    DEFAULT_COMPRESSOR = 'blosclz4'
    DEFAULT_THREADS = 8
    DEFAULT_LEVEL = 4
    DEFAULT_SHUFFLE = 'shuffle'

    BLOSC_SHUFFLE = {
      'noshuffle': 0,
      'shuffle': 1,
      'bitshuffle': 2
    }
    BLOSC_COMPRESSOR = {
      'blosclz4': 0,
      'lz4': 1,
      'lz4hc': 2,
      'snappy': 3,
      'zlib': 4,
      'zstd': 5
    }
    
    def __init__(self, **kwargs):
        """
        Initialise the OdinDataAdapter object

        :param kwargs:
        """
        logging.debug("FPA init called")
        super(FPCompressionAdapter, self).__init__(**kwargs)
        
        self._compression = 'off'
        
        self._threads = self.DEFAULT_THREADS
        self._level = self.DEFAULT_LEVEL
        self._shuffle = self.DEFAULT_SHUFFLE
        self._compressor = self.DEFAULT_COMPRESSOR

        self._hdf = self.DEFAULT_HDF_PLUGIN
        self._blosc = self.DEFAULT_BLOSC_PLUGIN

        # Setup the parameter tree for the adapter
        self._param_tree = ParameterTree({
            'config': {
                'compression': (self.get_compression, self.set_compression, {
                    'allowed_values': ['off', 'on']
                })
            }
        })

    def get_compression(self):
      return self._compression
      
    def set_compression(self, value):
      # If compression is turned on then send the following parameters
      if value == 'on':
        # First set the FP applications into compression mode
        request = ApiAdapterRequest(json_encode('compression'))
        self.put('config/execute/index', request)

        # Now set the compression parameters on the blosc plugin
        request = ApiAdapterRequest(json_encode(self._threads))
        self.put('config/{}/threads'.format(self._blosc), request)
        request = ApiAdapterRequest(json_encode(self._level))
        self.put('config/{}/level'.format(self._blosc), request)
        request = ApiAdapterRequest(json_encode(self.BLOSC_SHUFFLE[self._shuffle]))
        self.put('config/{}/shuffle'.format(self._blosc), request)
        request = ApiAdapterRequest(json_encode(self.BLOSC_COMPRESSOR[self._compressor]))
        self.put('config/{}/compressor'.format(self._blosc), request)

        # Now set the compression parameters on the hdf plugin
        request = ApiAdapterRequest(json_encode(self.HDF_BLOSC_COMPRESSION))
        self.put('config/{}/dataset/data/compression'.format(self._hdf), request)
        request = ApiAdapterRequest(json_encode(self._level))
        self.put('config/{}/dataset/data/blosc_level'.format(self._hdf), request)
        request = ApiAdapterRequest(json_encode(self.BLOSC_SHUFFLE[self._shuffle]))
        self.put('config/{}/dataset/data/blosc_shuffle'.format(self._hdf), request)
        request = ApiAdapterRequest(json_encode(self.BLOSC_COMPRESSOR[self._compressor]))
        self.put('config/{}/dataset/data/blosc_compressor'.format(self._hdf), request)

      elif value == 'off':
        # First set the FP applications into no_compression mode
        request = ApiAdapterRequest(json_encode('no_compression'))
        self.put('config/execute/index', request)

        # Now switch off compression parameters on the hdf plugin
        request = ApiAdapterRequest(json_encode(self.HDF_NO_COMPRESSION))
        self.put('config/{}/dataset/data/compression'.format(self._hdf), request)

      
      self._compression = value

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
          index = path.rstrip('/').split('/')[-1]
          response = self._param_tree.get(path, with_metadata=True)[index]
          if index == 'config':
            response.update(super(FPCompressionAdapter, self).get(path, request).data)
        except ParameterTreeError as ex:
          # Pass the request onto the super class
          return super(FPCompressionAdapter, self).get(path, request)          

        return ApiAdapterResponse(response, status_code=status_code)

    @request_types('application/json', 'application/vnd.odin-native')
    @response_types('application/json', default='application/json')
    def put(self, path, request):  # pylint: disable=W0613

        """
        Implementation of the HTTP PUT verb for FPCompressionAdapter.
        
        This will intercept only the compression parameter and then construct
        all of the necessary messages.  All other parameters will be passed
        straight through to the super class

        :param path: URI path of the PUT request
        :param request: Tornado HTTP request object
        :return: ApiAdapterResponse object to be returned to the client
        """
        status_code = 200
        response = {}
        logging.debug("PUT path: %s", path)
        logging.debug("PUT request: %s", request)

        try:
          value = json_decode(request.body)
          if isinstance(value, unicode):
            value = value.encode("utf-8")
          self._param_tree.set(path, value)
        except ParameterTreeError as ex:
          # Pass the request onto the super class
          return super(FPCompressionAdapter, self).put(path, request)          

        return ApiAdapterResponse(response, status_code=status_code)

