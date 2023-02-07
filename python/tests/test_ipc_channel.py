from nose.tools import assert_equal
from odin_data.control.ipc_channel import IpcChannel

class TestIpcChannel:

    @classmethod
    def setup_class(cls):

        cls.endpoint = "inproc://rx_channel"
        cls.send_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_PAIR)
        cls.recv_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_PAIR)

        cls.send_channel.bind(cls.endpoint)
        cls.recv_channel.connect(cls.endpoint)

    @classmethod
    def teardown_class(cls):

        cls.recv_channel.close()
        cls.send_channel.close()

    def test_basic_send_receive(self):

        msg = "This is a test message"
        self.send_channel.send(msg)

        reply = self.recv_channel.recv()

        assert_equal(msg, reply)
        assert_equal(type(msg), type(reply))


    def test_dealer_router(self):

        endpoint = 'inproc://dr_channel'
        msg = "This is a dealer router message"
        dealer_indentity = 'test_dealer'

        dealer_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_DEALER,
                                    identity=dealer_indentity)
        router_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_ROUTER)

        router_channel.bind(endpoint)
        dealer_channel.connect(endpoint)

        dealer_channel.send(msg)
        (recv_identity, reply) = router_channel.recv()

        assert_equal(len(reply), 1)
        assert_equal(msg, reply[0])
        assert_equal(type(msg), type(reply[0]))
        assert_equal(dealer_indentity, recv_identity)
