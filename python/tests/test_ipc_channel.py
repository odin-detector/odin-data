import pytest
from odin_data.control.ipc_channel import IpcChannel

rx_endpoint = "inproc://rx_channel"


@pytest.fixture(scope="class")
def send_channel():
    channel = IpcChannel(IpcChannel.CHANNEL_TYPE_PAIR)
    channel.bind(rx_endpoint)
    yield channel
    channel.close()


@pytest.fixture(scope="class")
def recv_channel():
    channel = IpcChannel(IpcChannel.CHANNEL_TYPE_PAIR)
    channel.connect(rx_endpoint)
    yield channel
    channel.close()


class TestIpcChannel:
    def test_basic_send_receive(self, send_channel, recv_channel):
        msg = "This is a test message"
        send_channel.send(msg)

        reply = recv_channel.recv()

        assert msg == reply
        assert type(msg) == type(reply)

    def test_dealer_router(self):
        endpoint = "inproc://dr_channel"
        msg = "This is a dealer router message"
        dealer_indentity = "test_dealer"

        dealer_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_DEALER, identity=dealer_indentity)
        router_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_ROUTER)

        router_channel.bind(endpoint)
        dealer_channel.connect(endpoint)

        dealer_channel.send(msg)
        (recv_identity, reply) = router_channel.recv()

        assert len(reply) == 1
        assert msg == reply[0]
        assert type(msg) == type(reply[0])
        assert dealer_indentity == recv_identity
