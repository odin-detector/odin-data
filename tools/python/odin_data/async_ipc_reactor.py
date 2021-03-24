import asyncio
import zmq
import zmq.asyncio

class AsyncIpcReactorTimer():

    def __init__(self, delay, times, callback, loop=None):

        self.delay = delay
        self.times = times
        self.expired = False

        self.callback = callback

        if not loop:
            loop = asyncio.get_event_loop()
        self.task = loop.create_task(self._run())

    def get_id(self):
        return id(self)

    async def _run(self):

        while not self.expired:

            await asyncio.gather(
                # asyncio.create_task(self.callback()),
                self.callback(),
                asyncio.sleep(self.delay)
            )

            if self.times > 0:
                self.times -= 1
                self.expired = not self.times

class AsyncIpcReactor():

    def __init__(self):

        self.handlers = {}
        self.timers = {}
        self.poller = zmq.asyncio.Poller()

        self._terminate = False

    def register_channel(self, channel, handler, flags=zmq.POLLIN):

        self.register_socket(channel.socket, handler, flags)

    def remove_channel(self, channel):
        raise NotImplementedError

    def register_socket(self, socket, handler, flags=zmq.POLLIN):

        self.handlers[socket] = handler
        self.poller.register(socket, flags)

    def remove_socket(self, socket):
        raise NotImplementedError

    def register_timer(self, delay_ms, times, callback):

        timer = AsyncIpcReactorTimer(delay_ms, times, callback)
        timer_id = timer.get_id()

        self.timers[timer_id] = timer
        return timer_id

    def remove_timer(self, timer_id):
        raise NotImplementedError

    async def run(self, timeout_ms=-1):

        self._terminate = False
        while not self._terminate:

            events = await self.poller.poll(timeout=timeout_ms)
            for socket in dict(events):
                if socket in self.handlers:
                    await self.handlers[socket]()

    def start(self, loop=None, timeout_ms=-1):

        if not loop:
            loop = asyncio.get_event_loop()

        loop.create_task(self.run(timeout_ms))

    def stop(self):

        self._terminate = True