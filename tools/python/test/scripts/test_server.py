import asyncio
import zmq
import zmq.asyncio
import zmq.utils.monitor
import logging
import json
import signal

import click

from odin_data.ipc_message import IpcMessage, IpcMessageException

class AsyncRouterServer():

    def __init__(self, addr='127.0.0.1', port=5555):

        self.endpoint = f'tcp://{addr}:{port}'
        logging.info(f"Starting server at endpoint {self.endpoint}")

        self.ctx = zmq.asyncio.Context.instance()
        self.router = self.ctx.socket(zmq.ROUTER)
        self.router.bind(self.endpoint)

        self.monitor = self.router.get_monitor_socket()

        self.poller = zmq.asyncio.Poller()

        self.event_map = {}
        for name in dir(zmq):
            if name.startswith('EVENT_'):
                value = getattr(zmq, name)
                self.event_map[value] = name

    def handle_exception(self, loop, context):
        # context["message"] will always be there; but context["exception"] may not
        msg = context.get("exception", context["message"])
        logging.error(f"Caught exception: {msg}")
        logging.info("Shutting down...")
        asyncio.create_task(self.shutdown(loop))

    async def shutdown(self, loop, signal=None):
        """Cleanup tasks tied to the service's shutdown."""
        if signal:
            logging.info(f"Received exit signal {signal.name}...")
        tasks = [t for t in asyncio.all_tasks() if t is not
                asyncio.current_task()]

        [task.cancel() for task in tasks]

        logging.info(f"Cancelling {len(tasks)} outstanding tasks")
        await asyncio.gather(*tasks, return_exceptions=True)
        loop.stop()

    def run(self):

        loop = asyncio.get_event_loop()

        loop.set_exception_handler(self.handle_exception)
        signals = (signal.SIGHUP, signal.SIGTERM, signal.SIGINT)
        for s in signals:
            loop.add_signal_handler(
                s, lambda s=s: asyncio.create_task(self.shutdown(loop, signal=s)))

        try:
            loop.create_task(self.run_combined())
            #loop.create_task(self.run_idle())
            loop.run_forever()
        finally:
            loop.close()
            logging.info("Run completed")

    async def run_combined(self):

        self.poller.register(self.router, zmq.POLLIN)
        self.poller.register(self.monitor, zmq.POLLIN)

        while True:

            events = await self.poller.poll(timeout=250)
            events = dict(events)
            if self.router in events:
                await self.handle_router()
            elif self.monitor in events:
                await self.handle_monitor()
            else:
                pass
                #logging.debug(".")

    async def run_router(self):

        while True:

            pollevts = await self.router.poll(timeout=250)

            if pollevts == zmq.POLLIN:
                await self.handle_router()
            else:
                logging.debug(".")

    async def handle_router(self):

        recvd_msg = await self.router.recv_multipart()
        [client_id, msg] = [elem.decode('utf-8') for elem in recvd_msg]

        request = IpcMessage(from_str=msg)
        logging.info(f"Recvd: client_id {client_id} request {request}")

        if request.get_msg_val() == 'detonate':
            logging.debug("***** DETONATE - not sending reply ****")
            return

        reply = IpcMessage(
            msg_type=IpcMessage.ACK, msg_val=request.get_msg_val(), id=request.get_msg_id()
        )
        response = [elem.encode('utf-8') for elem in [client_id, reply.encode()]]
        await self.router.send_multipart(response)
        logging.info("Sent reply")


    async def run_monitor(self):

        while True:

            pollevts = await self.monitor.poll(timeout=1000)

            if pollevts == zmq.POLLIN:
                await self.handle_monitor()
            else:
                logging.debug("*")

    async def handle_monitor(self):

        recvd_msg = await self.monitor.recv_multipart()
        event = zmq.utils.monitor.parse_monitor_message(recvd_msg)
        event.update({'description': self.event_map[event['event']]})
        logging.debug(f"Router socket event: {event}")

    async def run_idle(self):

        while True:
            logging.debug(">")
            await asyncio.sleep(0.5)

@click.command()
@click.option('--addr', default='127.0.0.1', help='Endpoint address')
@click.option('--port', default=5555, help='Endpoint port')
def main(addr, port):

    logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)s %(message)s')

    server = AsyncRouterServer(addr, port)
    server.run()

if __name__ == '__main__':
    main()