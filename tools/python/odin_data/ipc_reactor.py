"""Implementation of odin_data inter-process communication channels.

This module implements the ODIN data IpcReactor class for single threaded timer and inter-process
communication via ZeroMQ sockets.

Alan Greer, OSL
"""
import zmq
import datetime
import logging
from ipc_message import IpcMessage


class IpcReactorTimer:
    """Timer class for IpcReactor.

    This class manages the timeout period and time of fire for individual timers.
    Maintaining the management within this class keeps the IpcReactor class clean.
    """

    last_timer_id = 0

    def __init__(self, delay_ms, times, callback):
        """Initalise the Timer.

        :param delay_ms: number of milliseconds between timer executions
        :param times: how many times to run the timer (0 means forever)
        :param callback: the callback method to call when the timer executes
        """
        self._log = logging.getLogger(".".join([__name__, self.__class__.__name__]))
        IpcReactorTimer.last_timer_id += 1
        self._timer_id = IpcReactorTimer.last_timer_id
        self._delay_ms = delay_ms
        self._times = times
        self._callback = callback
        self._when = self.clock_mono_ms() + delay_ms
        self._expired = False

    def get_id(self):
        """Return the timer ID."""
        return self._timer_id

    def do_callback(self):
        """Execute the callback method."""
        self._callback()

        if (self._times > 0) and ((self._times - 1) == 0):
            self._expired = True
        else:
            self._when += self._delay_ms

    def has_fired(self):
        """Return boolean of whether the timer has fired or not."""
        return IpcReactorTimer.clock_mono_ms() >= self._when

    def has_expired(self):
        """Return boolean of whether the timer has expired."""
        return self._expired

    def when(self):
        """Return the next time that the timer will fire (in ms)."""
        return self._when

    @staticmethod
    def clock_mono_ms():
        """Return the current time in milliseconds."""
        time = datetime.datetime.now()
        time_ms = int(time.strftime("%s")) * 1000.0 + int(time.microsecond / 1000.0)
        #log.debug("%12d", time_ms)
        return time_ms


class IpcReactor:
    """Main IpcReactor class.

    This class provides single threaded management of timers and sockets.
    Timers and channels can be added, and channels can be removed from the reactor.
    The reactor polls channels for messages and calculates the required dead time before the next
    timer fires.  This improves the efficiency as there is no constant poll loop running.
    """

    def __init__(self):
        """Initalise the Rector class."""
        logging.basicConfig(format='%(asctime)-15s %(message)s')
        self._log = logging.getLogger(".".join([__name__, self.__class__.__name__]))
        self._log.setLevel(logging.ERROR)
        self._terminate_reactor = False
        self._pollitems = {}
        self._callbacks = {}
        self._timers = {}
        self._pollsize = 0
        self._needs_rebuild = True
        self._channels = {}

    def register_channel(self, channel, callback):
        """Register a new channel with the reactor.

        :param channel: the channel to listen to
        :param callback: the callback method to call when a message arrives on the channel
        """
        self._log.debug("Registering channel: %s", channel)
        # Add channel to channel map
        self._channels[channel.socket] = channel
        self._callbacks[channel.socket] = callback
        # Signal a rebuild is required
        self._needs_rebuild = True

    def remove_channel(self, channel):
        """Remove a registered channel from the reactor.

        :param channel: the channel to remove
        """
        # Remove the channel from the map
        self._channels.pop(channel.socket)
        # Signal a rebuild is required
        self._needs_rebuild = True

    def register_timer(self, delay_ms, times, callback):
        """Register a new timer with the reactor.

        :param delay_ms: the delay between individual executions of the timer
        :param times: the number of times to execute the timer callback
        :param callback: the method to call when the timer fires
        """
        timer = IpcReactorTimer(delay_ms, times, callback)
        self._timers[timer.get_id()] = timer
        return timer.get_id()

    def run(self):
        """Start execution of the Reactor.  This method blocks until the Reactor is shut down."""
        rc = 0

        # Loop until the terminate flag is set
        while not self._terminate_reactor:
            # If the poll items list needs rebuilding, do it now
            if self._needs_rebuild:
                self.rebuild_pollitems()

            # If there are no channels to poll and no timers currently active, break out of the
            # reactor loop cleanly
            if self._pollsize == 0:
                rc = 0
                break

            try:
                # Poll the registered channels, using the tickless timeout based
                # on the next pending timer
                self._log.debug("Timeout: %s", self.calculate_timeout())
                pollrc = dict(self._poller.poll(self.calculate_timeout()))
                self._log.debug("%s", self._poller)

                for sock in pollrc:
                    if pollrc[sock] == zmq.POLLIN:
                        try:
                            channel_id, reply = self._channels[sock].recv()
                            self._log.debug("Message: %s", reply[0])
                            msg = IpcMessage(from_str=reply[0])
                            self._callbacks[sock](msg, channel_id)
                        except Exception as e:
                            # TODO: How to handle an exception here
                            self._log.debug("Caught reactor exception")
                            #self._log.exception(e)

                for timer in self._timers:
                    if self._timers[timer].has_fired():
                        self._timers[timer].do_callback()
                    if self._timers[timer].has_expired():
                        self._timers.pop(timer)

            except KeyboardInterrupt:
                break
            except Exception as e:
                self._log.exception(e)
                break

        return rc

    def rebuild_pollitems(self):
        """When a new channel is added for monitoring we need to rebuild this structure."""
        self._log.debug("Rebuilding pollitems...")
        # If the existing pollitems array is valid, delete it
        self._poller = zmq.Poller()

        self._pollsize = len(self._channels)
        self._log.debug("Number of channels: %d", self._pollsize)

        if self._pollsize > 0:
            for channel in self._channels:
                self._log.debug("Registering %s for polling", channel)
                self._poller.register(channel, zmq.POLLIN)

        self._needs_rebuild = False

    def calculate_timeout(self):
        """Calculate shortest timeout up to one hour (!!), looping through
        current timers to see which fires first"""
        tickless = IpcReactorTimer.clock_mono_ms() + (1000 * 3600)
        for timer in self._timers:
            if tickless > self._timers[timer].when():
                tickless = self._timers[timer].when()

        # Calculate current timeout based on that, set to zero (don't wait) if
        # there is no timers pending
        timeout = tickless - IpcReactorTimer.clock_mono_ms()
        if timeout < 0:
            timeout = 0
        return timeout
