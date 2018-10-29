"""Implementation of odin_data Meta Listener

This module listens on one or more ZeroMQ sockets for meta messages which it then passes
on to a MetaWriter to write. Also handles odin Ipc control messages.

Matt Taylor, Diamond Light Source
"""
import zmq
import logging
from odin_data.ipc_message import IpcMessage


class MetaListener:
    """Meta Listener class.

    This class listens on ZeroMQ sockets for incoming cnotrol and meta data messages
    """

    def __init__(self, directory, inputs, ctrl, writer_module):
        """Initalise the MetaListener object.

        :param directory: Directory to create the meta file in
        :param inputs: Comma separated list of input ZMQ addresses
        :param ctrl: Port to use for control messages
        :param writer_module: Detector writer class
        """
        self._inputs = inputs
        self._directory = directory
        self._ctrl_port = str(ctrl)
        self._writer_module = writer_module
        self._writers = {}
        self._kill_requested = False

        # create logger
        self.logger = logging.getLogger('meta_listener')

    def run(self):
        """Main application loop."""
        self.logger.info('Starting Meta listener...')

        receiver_list = []
        context = zmq.Context()
        ctrl_socket = None

        try:
            inputs_list = self._inputs.split(',')

            # Control socket
            ctrl_address = "tcp://*:" + self._ctrl_port
            self.logger.info('Binding control address to ' + ctrl_address)
            ctrl_socket = context.socket(zmq.ROUTER)
            ctrl_socket.bind(ctrl_address)

            # Socket to receive messages on
            for x in inputs_list:
                new_receiver = context.socket(zmq.SUB)
                new_receiver.connect(x)
                new_receiver.setsockopt(zmq.SUBSCRIBE, '')
                receiver_list.append(new_receiver)

            poller = zmq.Poller()
            for eachReceiver in receiver_list:
                poller.register(eachReceiver, zmq.POLLIN)

            poller.register(ctrl_socket, zmq.POLLIN)

            self.logger.info('Listening to inputs ' + str(inputs_list))

            while self._kill_requested == False:
                socks = dict(poller.poll())
                for receiver in receiver_list:
                    if socks.get(receiver) == zmq.POLLIN:
                        self.handle_message(receiver)

                if socks.get(ctrl_socket) == zmq.POLLIN:
                    self.handle_control_message(ctrl_socket)

            self.stop_all_writers()

            self.logger.info('Finished listening')
        except Exception as err:
            self.logger.error('Unexpected Exception: ' + str(err))

        # Finished
        for receiver in receiver_list:
            receiver.close(linger=0)

        if ctrl_socket is not None:
            ctrl_socket.close(linger=100)

        context.term()

        self.logger.info("Finished run")
        return

    def handle_control_message(self, receiver):
        """Handle control message.

        :param: receiver: ZeroMQ channel to receive message from
        """
        channel_id = receiver.recv()
        message_val = ""
        message_id = 0

        try:
            message = IpcMessage(from_str=receiver.recv())
            message_val = message.get_msg_val()
            message_id = message.get_msg_id()

            if message.get_msg_val() == 'status':
                reply = self.handle_status_message(message_id)
            elif message.get_msg_val() == 'request_configuration':
                reply = self.handle_request_config_message(message_id)
            elif message.get_msg_val() == 'configure':
                self.logger.debug('handling control configure message')
                self.logger.debug(message)
                params = message.attrs['params']
                reply = self.handle_configure_message(params, message_id)
            else:
                reply = IpcMessage(IpcMessage.NACK, message_val, id=message_id)
                reply.set_param('error', 'Unknown message value type')

        except Exception as err:
            self.logger.error('Unexpected Exception handling control message: ' + str(err))
            reply = IpcMessage(IpcMessage.NACK, message_val, id=message_id)
            reply.set_param('error', 'Error processing control message')

        receiver.send(channel_id, zmq.SNDMORE)
        receiver.send(reply.encode())

    def handle_status_message(self, msg_id):
        """Handle status message.

        :param: msg_id: message id to use for reply
        """
        status_dict = {}
        for key in self._writers:
            writer = self._writers[key]
            status_dict[key] = {'filename': writer.full_file_name, 'num_processors': writer.number_processes_running,
                                'written': writer.write_count, 'writing': not writer.finished}
            writer.write_timeout_count = writer.write_timeout_count + 1

        reply = IpcMessage(IpcMessage.ACK, 'status', id=msg_id)
        reply.set_param('acquisitions', status_dict)

        # Now delete any finished acquisitions, and stop any stagnant ones
        for key, value in self._writers.items():
            if value.finished:
                del self._writers[key]
            else:
                if value.number_processes_running == 0 and value.write_timeout_count > 10 and value.file_created:
                    self.logger.info('Force stopping stagnant acquisition: ' + str(key))
                    value.stop()

        return reply

    def handle_request_config_message(self, msg_id):
        """Handle request config message.

        :param: msg_id: message id to use for reply
        """
        acquisitions_dict = {}
        for key in self._writers:
            writer = self._writers[key]
            acquisitions_dict[key] = {'output_dir': writer.directory, 'flush': writer.flush_frequency}

        reply = IpcMessage(IpcMessage.ACK, 'request_configuration', id=msg_id)
        reply.set_param('acquisitions', acquisitions_dict)
        reply.set_param('inputs', self._inputs)
        reply.set_param('default_directory', self._directory)
        reply.set_param('ctrl_port', self._ctrl_port)
        return reply

    def handle_configure_message(self, params, msg_id):
        """Handle configure message.

        :param: params: dictionary of configuration parameters
        :param: msg_id: message id to use for reply
        """
        reply = IpcMessage(IpcMessage.NACK, 'configure', id=msg_id)
        reply.set_param('error', 'Unable to process configure command')

        if 'kill' in params:
            self.logger.info('Kill reqeusted')
            reply = IpcMessage(IpcMessage.ACK, 'configure', id=msg_id)
            self._kill_requested = True
        elif 'acquisition_id' in params:
            acquisition_id = params['acquisition_id']

            if acquisition_id is not None:
                if acquisition_id in self._writers:
                    self.logger.debug('Writer is in writers for acq ' + str(acquisition_id))
                else:
                    self.logger.debug('Writer not in writers for acquisition [' + str(acquisition_id) + ']')
                    self.logger.info(
                        'Creating new acquisition [' + str(acquisition_id) + '] with default directory ' + str(
                            self._directory))
                    self.create_new_acquisition(self._directory, acquisition_id)

                if 'output_dir' in params:
                    self.logger.debug('Setting acquisition [' + str(acquisition_id) + '] directory to ' + str(
                        params['output_dir']))
                    self._writers[acquisition_id].directory = params['output_dir']
                    reply = IpcMessage(IpcMessage.ACK, 'configure', id=msg_id)

                if 'flush' in params:
                    self.logger.debug(
                        'Setting acquisition [' + str(acquisition_id) + '] flush to ' + str(params['flush']))
                    self._writers[acquisition_id].flush_frequency = params['flush']
                    reply = IpcMessage(IpcMessage.ACK, 'configure', id=msg_id)

                if 'flush_timeout' in params:
                    self.logger.debug('Setting acquisition [' + str(acquisition_id) + '] flush timeout to ' + str(
                        params['flush_timeout']))
                    self._writers[acquisition_id].flush_timeout = params['flush_timeout']
                    reply = IpcMessage(IpcMessage.ACK, 'configure', id=msg_id)

                if 'stop' in params:
                    self.logger.info('Stopping acquisition [' + str(acquisition_id) + ']')
                    self._writers[acquisition_id].stop()
                    reply = IpcMessage(IpcMessage.ACK, 'configure', id=msg_id)
            else:
                # If the command is to stop without an acqID then stop all acquisitions
                if 'stop' in params:
                    self.stop_all_writers()
                    reply = IpcMessage(IpcMessage.ACK, 'configure', id=msg_id)
                else:
                    reply = IpcMessage(IpcMessage.NACK, 'configure', id=msg_id)
                    reply.set_param('error', 'Acquisition ID was None')

        else:
            reply = IpcMessage(IpcMessage.NACK, 'configure', id=msg_id)
            reply.set_param('error', 'No params in config')
        return reply

    def handle_message(self, receiver):
        """Handle a meta data message.

        :param: receiver: ZeroMQ channel to read message from
        """
        self.logger.debug('Handling message')

        try:
            message = receiver.recv_json()
            self.logger.debug(message)
            userheader = message['header']

            if 'acqID' in userheader:
                acquisition_id = userheader['acqID']
            else:
                self.logger.warn('Didnt have acquisition id in header')
                acquisition_id = ''

            if acquisition_id not in self._writers:
                self.logger.error('No writer for acquisition [' + acquisition_id + ']')
                receiver.recv()
                return

            writer = self._writers[acquisition_id]

            if writer.finished:
                self.logger.error('Writer finished for acquisition [' + acquisition_id + ']')
                receiver.recv()
                return

            writer.process_message(message, userheader, receiver)

        except Exception as err:
            self.logger.error('Unexpected Exception handling message: ' + str(err))

        return

    def create_new_acquisition(self, directory, acquisition_id):
        """Create a new writer to handle meta messages for a new acquisition.

        :param: directory: Directory to create the meta file in
        :param: acquisition_id: Acquisition ID of the new acquisition
        """
        # First finish any acquisition that is queued up already which hasn't started
        for key, value in self._writers.items():
            if value.finished == False and value.number_processes_running == 0 and value.file_created == False:
                self.logger.info('Force finishing unused acquisition: ' + str(key))
                value.finished = True

        # Now create new acquisition
        self.logger.info('Creating new acquisition for: ' + str(acquisition_id))
        self._writers[acquisition_id] = self.create_new_writer(directory, acquisition_id)

        # Then check if we have built up too many finished acquisitions and delete them if so
        if len(self._writers) > 3:
            for key, value in self._writers.items():
                if value.finished:
                    del self._writers[key]
                    
    def stop_all_writers(self):
        """Force stop all writers."""
        for key in self._writers:
            self.logger.info('Forcing close of writer for acquisition: ' + str(key))
            writer = self._writers[key]
            writer.stop()

    def create_new_writer(self, directory, acquisition_id):
        """Create a the appropriate writer object.

        :param: directory: Directory to create the meta file in
        :param: acquisition_id: Acquisition ID of the new acquisition
        """
        module_name = self._writer_module[:self._writer_module.rfind('.')]
        class_name = self._writer_module[self._writer_module.rfind('.') + 1:]
        module = __import__(module_name)
        writer_class = getattr(module, class_name)
        writer_instance = writer_class(self.logger, directory, acquisition_id)
        self.logger.debug(writer_instance)
        return writer_instance

