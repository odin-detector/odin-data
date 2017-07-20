from common import ZMQClient


class FrameReceiverClient(ZMQClient):

    CTRL_PORT = 5000
    FAN_ENDPOINT = "tcp://{}:5559"

    def __init__(self, ip_address, lock, server_rank=0):
        super(FrameReceiverClient, self).__init__(ip_address, lock,
                                                  server_rank)

    def request_status(self):
        response = self.send_request("status")
        return response.attrs

    def configure_shared_memory(self, shared_memory, ready, release):
        config = {
            "fr_shared_mem": shared_memory,
            "fr_ready_cnxn": ready,
            "fr_release_cnxn": release
        }
        # self.send_configuration(self.SHARED_MEMORY_SETUP, config)
