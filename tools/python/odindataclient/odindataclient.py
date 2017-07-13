from frameprocessorclient import FrameProcessorClient
from framereceiverclient import FrameReceiverClient


class OdinDataClient(object):

    SHARED_MEMORY = "OdinDataBuffer"
    READY = "tcp://127.0.0.1:{}"
    RELEASE = "tcp://127.0.0.1:{}"

    def __init__(self, rank, processes, ip_address, lock, server_rank=0):
        self.receiver = FrameReceiverClient(ip_address, lock, server_rank)
        self.processor = FrameProcessorClient(rank, processes,
                                              ip_address, lock, server_rank)

        self.shared_memory = self.SHARED_MEMORY + str(server_rank + 1)
        base_port = 5000 + server_rank * 1000
        self.ready = self.READY.format(base_port + 1)
        self.release = self.RELEASE.format(base_port + 2)

    def configure_shared_memory(self):
        self.processor.configure_shared_memory(self.shared_memory,
                                               self.ready, self.release)
        self.receiver.configure_shared_memory(self.shared_memory,
                                              self.ready, self.release)

    def request_status(self):
        status = self.processor.request_status()
        status.update(self.receiver.request_status())

        return status
