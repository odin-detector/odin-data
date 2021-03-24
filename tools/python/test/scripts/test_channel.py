import asyncio
from odin_data.async_ipc_channel import AsyncIpcChannel, IpcChannelException
from odin_data.ipc_message import IpcMessage, IpcMessageException

async def run_channel():

    ctrl_endpoint = 'tcp://127.0.0.1:5555'

    ctrl_channel = AsyncIpcChannel(AsyncIpcChannel.CHANNEL_TYPE_DEALER)
    ctrl_channel.connect(ctrl_endpoint)

    message_id = 0

    while True:
        msg = IpcMessage("cmd", "configure", id=message_id)
        await ctrl_channel.send(msg.encode())
        print(".")
        await asyncio.sleep(1.0)
        message_id += 1

def main():

    loop = asyncio.get_event_loop()

    loop.run_until_complete(
        run_channel()
    )

if __name__ == '__main__':

    main()