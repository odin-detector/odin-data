import logging
from threading import RLock

from odin_data.ipc_tornado_channel import IpcTornadoChannel
from odin_data.ipc_message import IpcMessage, IpcMessageException
from datetime import datetime


class IpcTornadoClient(object):

    ENDPOINT_TEMPLATE = "tcp://{IP}:{PORT}"

    MESSAGE_ID_MAX = 2**32

    def __init__(self, ip_address, port):
        self.logger = logging.getLogger(self.__class__.__name__)

        self._ip_address = ip_address
        self._port = port

        self.ctrl_endpoint = self.ENDPOINT_TEMPLATE.format(IP=ip_address, PORT=port)
        self.logger.debug("Connecting to client at %s", self.ctrl_endpoint)
        self.ctrl_channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_DEALER)
        self.ctrl_channel.connect(self.ctrl_endpoint)
        self.ctrl_channel.register_callback(self._callback)
        self._parameters = {}
        self.message_id = 0

        self._lock = RLock()

    @property
    def parameters(self):
        return self._parameters

    def _callback(self, msg):
        # Handle the multi-part message
        self.logger.debug("Msg received: %s", msg)
        reply = IpcMessage(from_str=msg[0])
        if 'request_configuration' in reply.get_msg_val():
            self._update_configuration(reply.attrs)
        if 'status' in reply.get_msg_val():
            self._update_status(reply.attrs)

    def _update_configuration(self, config_msg):
        params = config_msg['params']
        self._parameters['config'] = params
        logging.debug("Current configuration updated to: %s", self._parameters['config'])

    def _update_status(self, status_msg):
        params = status_msg['params']
        params['connected'] = True
        params['timestamp'] = status_msg['timestamp']
        self._parameters['status'] = params
        logging.debug("Status updated to: %s", self._parameters['status'])

    def check_for_stale_status(self, max_stale_time):
        if 'status' in self._parameters:
            # Check for the timestamp
            if 'timestamp' in self._parameters['status']:
                ts = datetime.strptime(self._parameters['status']['timestamp'], "%Y-%m-%dT%H:%M:%S.%f")
                ttlm = datetime.now() - ts
                if ttlm.seconds > max_stale_time:
                    # This connection has gone stale, set connected to false
                    self._parameters['status'] = {'connected': False}
                    logging.debug("Status updated to: %s", self._parameters['status'])
        else:
            # No status for this client so set connected to false
            self._parameters['status'] = {'connected': False}
            logging.debug("Status updated to: %s", self._parameters['status'])


    def _send_message(self, msg):
        msg.set_msg_id(self.message_id)
        self.message_id = (self.message_id + 1) % self.MESSAGE_ID_MAX
        self.logger.debug("Sending control message:\n%s", msg.encode())
        with self._lock:
            self.ctrl_channel.send(msg.encode())

    @staticmethod
    def _raise_reply_error(msg, reply):
        if reply is not None:
            raise IpcMessageException("Request\n%s\nunsuccessful. Got invalid response: %s" % (msg, reply))
        else:
            raise IpcMessageException("Request\n%s\nunsuccessful. Got no response." % msg)

    def send_request(self, value):
        msg = IpcMessage("cmd", value)
        self._send_message(msg)

    def send_configuration(self, content, target=None, valid_error=None):
        msg = IpcMessage("cmd", "configure")

        if target is not None:
            msg.set_param(target, content)
        else:
            for parameter, value in content.items():
                msg.set_param(parameter, value)

        self._send_message(msg)


