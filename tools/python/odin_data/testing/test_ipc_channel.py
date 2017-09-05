from frame_receiver.ipc_channel import IpcChannel, IpcChannelException
from nose.tools import assert_equal

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