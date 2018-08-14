"""
Created on 2nd August 2018

:author: Alan Greer
"""
import copy
import logging
from odin_data.odin_data_adapter import OdinDataAdapter


class FrameReceiverAdapter(OdinDataAdapter):
    """
    OdinDataAdapter class

    This class provides the adapter interface between the ODIN server and the ODIN-DATA detector system,
    transforming the REST-like API HTTP verbs into the appropriate frameProcessor ZeroMQ control messages
    """
    def __init__(self, **kwargs):
        """
        Initialise the OdinDataAdapter object

        :param kwargs:
        """
        logging.debug("FrameReceiverAdapter init called")

        super(FrameReceiverAdapter, self).__init__(**kwargs)

        self._decoder_config = []
        for ep in self._endpoints:
            self._decoder_config.append(None)

    def send_to_clients(self, request_command, parameters, client_index=-1):
        """
        Intercept the base class send_to_clients method.
        Keep a record of any decoder specific configuration items and then if a single decoder config
        item is later changed send the full decoder configuration to the Frame Receiver application.
        This is necessary as often a decoder config change will result in the complete tear down and re-init
        of the Decoder class and so a full and consistent set of decoder config parameters are required.

        :param request_command:
        :param parameters:
        :param client_index:
        """
        logging.debug("Original index: {} request_command: {} and parameters: {}".format(client_index, request_command, parameters))
        command, parameters = self.uri_params_to_dictionary(request_command, parameters)

        decoder_config = None
        if command is None:
            if 'decoder_config' in parameters:
                logging.debug("Found decoder config: {}".format(parameters['decoder_config']))
                decoder_config = parameters['decoder_config']
        elif command == 'decoder_config':
                logging.debug("Found decoder config: {}".format(parameters))
                decoder_config = parameters

        if decoder_config is not None:
            if client_index == -1:
                for index in range(len(self._decoder_config)):
                    if self._decoder_config[index] is None:
                        self._decoder_config[index] = decoder_config
                    else:
                        for item in decoder_config:
                            self._decoder_config[index][item] = decoder_config[item]
            else:
                if self._decoder_config[client_index] is None:
                    self._decoder_config[client_index] = decoder_config
                else:
                    for item in decoder_config:
                        self._decoder_config[client_index][item] = decoder_config[item]

        # Now construct the new full message, inserting the full decoder config back into the parameters
        if client_index == -1:
            new_param_set = []
            # Loop over each decoder config item and send a list of commands
            for dc in self._decoder_config:
                if command is None:
                    if 'decoder_config' in parameters and dc is not None:
                        parameters['decoder_config'] = dc
                elif command == 'decoder_config':
                    parameters = dc
                new_param_set.append(copy.deepcopy(parameters))
        else:
            if command is None:
                if 'decoder_config' in parameters and self._decoder_config[client_index] is not None:
                    parameters['decoder_config'] = self._decoder_config[client_index]
            elif command == 'decoder_config':
                parameters = self._decoder_config[client_index]
            new_param_set = parameters

        logging.debug("Updated full command: {} and parameter set: {}".format(command, new_param_set))

        return super(FrameReceiverAdapter, self).send_to_clients(command, new_param_set, client_index)
