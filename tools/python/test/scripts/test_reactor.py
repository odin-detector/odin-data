import asyncio

from odin_data.async_ipc_reactor import AsyncIpcReactor
from odin_data.async_ipc_client import AsyncIpcClient
from odin_data.ipc_message import IpcMessage

class ReactorTest():

    def __init__(self, ip_addr, port):

        self.client = AsyncIpcClient(ip_addr, port)
        self.reactor = AsyncIpcReactor()

        self.reactor.register_channel(self.client.ctrl_channel, self.handle_reply)
        self.reactor.register_timer(1000, 0, self.run_client)

    def run(self):

        self.reactor.start()
        asyncio.get_event_loop().run_forever()

    async def run_client(self):

        config = {'param_one': 'value'}
        await self.client.send_configuration(config)

    async def handle_reply(self):

        msg = await self.client.ctrl_channel.recv()
        reply = IpcMessage(from_str=msg)
        print(f"Got reply {reply}")

def main():

    ip_addr = '127.0.0.1'
    port = 5555

    rt = ReactorTest(ip_addr, port)
    rt.run()

if __name__ == '__main__':

    main()