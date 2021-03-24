import logging
import time
import asyncio
import json
from functools import partial

from tornado.escape import json_decode

from odin.adapters.adapter import ApiAdapterResponse, request_types, response_types
from odin.adapters.async_adapter import AsyncApiAdapter
from odin._version import get_versions

from odin_data.async_ipc_client import AsyncIpcClient
from odin_data.ipc_message import IpcMessage
from odin_data.util import remove_prefix, remove_suffix


class DemoAsyncAdapter(AsyncApiAdapter):

    def __init__(self, **kwargs):

        super(DemoAsyncAdapter, self).__init__(**kwargs)

        endpoints = self.options.get('endpoints', '127.0.0.1:5555')
        update_interval = float(self.options.get('update_interval', 1.0))

        self.async_demo = DemoAsync(endpoints, update_interval)

        logging.debug('DemoAsyncAdapter loaded')

    @request_types('application/json')
    @response_types('application/json', default='application/json')
    async def get(self, path, request):

        try:
            response = await self.async_demo.get(path)
            status_code = 200
        except DemoAsyncError as e:
            response = {'error': str(e)}
            status_code = 400

        content_type = 'application/json'
        return ApiAdapterResponse(
            response, content_type=content_type, status_code=status_code
        )

    @request_types('application/json', 'application/vnd.odin-native')
    @response_types('application/json', default='application/json')
    async def put(self, path, request):

        try:
            data = json_decode(request.body)
            response = await self.async_demo.set(path, data)
            #response = self.workshop.get(path)
            response = {'hey': 'there', 'path': path}
            status_code = 200
        except DemoAsyncError as e:
            response = {'error': str(e)}
            status_code = 400
        except (TypeError, ValueError) as e:
            response = {'error': 'Failed to decode PUT request body: {}'.format(str(e))}
            status_code = 400

        return ApiAdapterResponse(response, status_code=status_code)

class DemoAsyncError(Exception):
    pass

class DemoAsync():

    def __init__(self, endpoints, update_interval=1.0):

        self.endpoints = []
        # Parse endpoint configuration parameter into a list of client endpoints
        for arg in endpoints.split(','):
            arg_elems = arg.strip().split(':')
            self.endpoints.append((str(arg_elems[0]), int(arg_elems[1])))

        self.update_interval = update_interval

        self.clients = []

        for endpoint in self.endpoints:
            client = AsyncIpcClient(*endpoint)
            self.clients.append(client)

        for client in self.clients:
            client.start_update_loop(self.update_interval)

    async def do_update(self):

        requests = []
        for client in self.clients:
            for request in ["status", "request_configuration"]:
                requests.append(client.send_request(request))

        await asyncio.gather(*requests)

    async def get(self, path):

        response = {
            'path': path,
            'clients': [client.parameters for client in self.clients]
        }
        return response

    async def set(self, path, data):

        logging.debug(f"DemoAsync PUT on path {path} with data {data}")

        path = path.strip('/')

        if path.startswith("command/"):
            command = remove_prefix(path, "command/")
            logging.debug(f"Request is for command {command}")

            requests = []
            for client in self.clients:
                requests.append(client.send_request(command, await_response=True))

            await asyncio.gather(*requests)
