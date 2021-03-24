import logging
import json
import asyncio

from odin_data.async_ipc_channel import AsyncIpcChannel
from odin_data.ipc_message import IpcMessage, IpcMessageException

class AsyncIpcClient(object):

    ENDPOINT_TEMPLATE = "tcp://{IP}:{PORT}"

    MESSAGE_ID_MAX = 2**32

    def __init__(self, ip_address, port, ioloop=None):

        self._ip_address = ip_address
        self._port = port
        self._ioloop = ioloop

        self._update_task = None
        self.message_id = 0
        self._parameters = {
            'status': {'connected': False, 'timeout': False}
        }

        self.logger = logging.getLogger(self.__class__.__name__)

        self.ctrl_endpoint = self.ENDPOINT_TEMPLATE.format(IP=ip_address, PORT=port)
        self.logger.debug("Connecting to client at %s", self.ctrl_endpoint)
        self.ctrl_channel = AsyncIpcChannel(AsyncIpcChannel.CHANNEL_TYPE_DEALER)

        self.ctrl_channel.enable_monitor()
        if not self._ioloop:
            self._ioloop = asyncio.get_event_loop()
        self._monitor_task = self._ioloop.create_task(self._run_channel_monitor())

        self.ctrl_channel.connect(self.ctrl_endpoint)

    @property
    def parameters(self):
        return self._parameters

    @property
    def connected(self):
        return self._parameters['status']['connected']

    def start_update_loop(self, update_interval):

        async def run_update_loop():
            while True:
                await asyncio.gather(
                    self._do_client_update(),
                    asyncio.sleep(update_interval)
                )

        if not self._ioloop:
            self._ioloop = asyncio.get_event_loop()
        self._update_task = self._ioloop.create_task(run_update_loop())

    async def _do_client_update(self):

        for request in ["status", "request_configuration"]:
            await asyncio.gather(
                self.send_request(request),
                self.receive_reply(),
            )

    async def _send_message(self, msg):

        msg.set_msg_id(self.message_id)
        self.message_id = (self.message_id + 1) % self.MESSAGE_ID_MAX
        self.logger.debug("Sending control message [%s]:\n%s", self.ctrl_endpoint, msg.encode())
        await self.ctrl_channel.send(msg.encode())

    async def receive_reply(self, timeout=0):

        if timeout:
            timeout_ms = int(timeout * 1000)
            pollevts = await self.ctrl_channel.poll(timeout_ms)
            if pollevts != AsyncIpcChannel.POLLIN:
                self.logger.error(f"Timeout receiving response from client {self.ctrl_endpoint}")
                self._parameters['status']['timeout'] = True
                return None

        msg = await self.ctrl_channel.recv()
        reply = IpcMessage(from_str=msg)
        self.logger.debug(f"Received response from client {self.ctrl_endpoint}: {reply}")

        self._parameters['status']['timeout'] = False
        self._parameters['reply'] = json.loads(msg)
        if 'request_version' in reply.get_msg_val():
            self._update_versions(reply.attrs)
        if 'request_configuration' in reply.get_msg_val():
            self._update_configuration(reply.attrs)
        if 'status' in reply.get_msg_val():
            self._update_status(reply.attrs)

        return reply

    async def _run_channel_monitor(self):

        while True:
            event = await self.ctrl_channel.recv_monitor()
            self.logger.debug("Monitor event received from %s: %s", self.ctrl_endpoint, event)
            if event['event'] == AsyncIpcChannel.EVENT_CONNECTED:
                self._parameters['status']['connected'] = True
            if event['event'] == AsyncIpcChannel.EVENT_DISCONNECTED:
                self._parameters['status']['connected'] = False

    def _update_versions(self, version_msg):
        params = version_msg['params']
        self._parameters['version'] = params['version']

    def _update_configuration(self, config_msg):
        params = config_msg['params']
        self._parameters['config'] = params

    def _update_status(self, status_msg):
        params = status_msg['params']
        params['timestamp'] = status_msg['timestamp']
        self._parameters['status'] = params
        # If we have received a status response then we must be connected
        self._parameters['status']['connected'] = True

    @staticmethod
    def _raise_reply_error(msg, reply):
        if reply is not None:
            raise IpcMessageException("Request\n%s\nunsuccessful. Got invalid response: %s" % (msg, reply))
        else:
            raise IpcMessageException("Request\n%s\nunsuccessful. Got no response." % msg)

    async def send_request(self, value, await_response=False):

        msg = IpcMessage("cmd", value)
        await self._send_message(msg)

        if await_response:
            if value == 'detonate':
                self.logger.debug("Going to send a detonate command")
                timeout = 1
            elif value == 'pop':
                self.logger.debug("Going to send a pop command")
                timeout = 1
            else:
                timeout = 0

            reply = await self.receive_reply(timeout=timeout)

    async def send_configuration(self, content, target=None, valid_error=None):
        msg = IpcMessage("cmd", "configure")

        if target is not None:
            msg.set_param(target, content)
        else:
            for parameter, value in content.items():
                msg.set_param(parameter, value)

        await self._send_message(msg)


