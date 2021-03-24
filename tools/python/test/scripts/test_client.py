import asyncio
from odin_data.async_ipc_client import AsyncIpcClient
from odin_data.ipc_message import IpcMessage, IpcMessageException

async def run_client():

    ip_addr = '127.0.0.1'
    port = 5555

    client = AsyncIpcClient(ip_addr, port)

    while True:
        config = {'param_one': 'value'}
        await client.send_configuration(config)
        msg = await client.ctrl_channel.recv()
        reply = IpcMessage(from_str=msg)
        print(f"Got reply {reply}")
        await asyncio.sleep(1.0)

def main():

    loop = asyncio.get_event_loop()

    loop.create_task(run_client())
    loop.run_forever()

if __name__ == '__main__':

    main()