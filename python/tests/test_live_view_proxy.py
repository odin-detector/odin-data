import logging
import sys

import pytest
from tornado.escape import json_encode

from odin_data.control.live_view_proxy_adapter import (
    DEFAULT_DEST_ENDPOINT,
    DEFAULT_DROP_WARN_PERCENT,
    DEFAULT_QUEUE_LENGTH,
    DEFAULT_SOURCE_ENDPOINT,
    Frame,
    LiveViewProxyAdapter,
    LiveViewProxyNode,
)

if sys.version_info[0] == 3:  # pragma: no cover
    from unittest.mock import Mock
else:  # pragma: no cover
    from mock import Mock


def log_message_seen(caplog, level, message, when="call"):
    for record in caplog.get_records(when):
        if record.levelno == level and message in record.getMessage():
            return True

    return False


@pytest.fixture()
def test_frames():
    frames = [Frame([json_encode({"frame_num": 1}), 0]), Frame([json_encode({"frame_num": 2}), 0])]
    return frames


class TestFrame:
    def test_frame_init(self, test_frames):
        assert test_frames[0].num == 1

    def test_frame_get_header(self, test_frames):
        assert test_frames[0].get_header() == json_encode({"frame_num": test_frames[0].num})

    def test_frame_get_data(self, test_frames):
        assert test_frames[0].data == 0

    def test_frame_set_acq(self, test_frames):
        test_frames[0].set_acq(5)
        assert test_frames[0].acq_id == 5

    def test_frame_order(self, test_frames):
        assert test_frames[0] < test_frames[1]

    def test_frame_order_acq(self, test_frames):
        test_frames[0].set_acq(2)
        test_frames[1].set_acq(1)

        assert test_frames[0] > test_frames[1]


@pytest.fixture()
def proxy_node_test():
    class ProxyNodeTest:
        def __init__(self):
            self.callback_count = 0
            self.node_1_info = {"name": "test_node_1", "endpoint": "tcp://127.0.0.1:5010"}
            self.node_2_info = {"name": "test_node_2", "endpoint": "tcp://127.0.0.1:5011"}
            self.node_1 = LiveViewProxyNode(
                self.node_1_info["name"], self.node_1_info["endpoint"], 0.5, self.callback
            )
            self.node_2 = LiveViewProxyNode(
                self.node_2_info["name"], self.node_2_info["endpoint"], 0.5, self.callback
            )

            self.test_frame = Frame([json_encode({"frame_num": 1}), "0"])

        def callback(self, frame, source):
            self.callback_count += 1

    return ProxyNodeTest()


class TestProxyNode(object):
    def test_node_init(self, proxy_node_test):
        assert proxy_node_test.node_1.name, proxy_node_test.node_1_info["name"]
        assert proxy_node_test.node_1.endpoint, proxy_node_test.node_1_info["endpoint"]

    def test_node_reset(self, proxy_node_test):
        proxy_node_test.node_1.received_frame_count = 10
        proxy_node_test.node_1.dropped_frame_count = 5
        proxy_node_test.node_1.has_warned = True
        proxy_node_test.node_1.set_reset()
        assert proxy_node_test.node_1.received_frame_count == 0
        assert proxy_node_test.node_1.dropped_frame_count == 0
        assert not proxy_node_test.node_1.has_warned

    def test_node_dropped_frame(self, proxy_node_test):
        proxy_node_test.node_1.received_frame_count = 10
        num_dropped = 9
        for _ in range(num_dropped):
            proxy_node_test.node_1.dropped_frame()

        assert proxy_node_test.node_1.dropped_frame_count == num_dropped
        assert proxy_node_test.node_1.has_warned

    def test_node_callback(self, proxy_node_test):
        test_header = {"frame_num": 1}
        for i in range(10):
            test_header["frame_num"] = i
            proxy_node_test.node_1.local_callback([json_encode(test_header), 0])
        test_header["frame_num"] = 1
        proxy_node_test.node_1.local_callback([json_encode(test_header), 0])

        assert proxy_node_test.node_1.current_acq == 1
        assert proxy_node_test.node_1.received_frame_count == 11
        assert proxy_node_test.node_1.last_frame == 1


@pytest.fixture(scope="class")
def config_node_list():
    return {
        "test_node_1": "tcp://127.0.0.1:5000",
        "test_node_2": "tcp://127.0.0.1:5001",
    }


@pytest.fixture(scope="class")
def adapter_config(config_node_list):
    config = {
        "destination_endpoint": "tcp://127.0.0.1:5021",
        "source_endpoints": ",\n".join(
            ["{}={}".format(key, val) for (key, val) in config_node_list.items()]
        ),
        "dropped_frame_warning_cutoff": 0.2,
        "queue_length": 15,
    }
    return config


@pytest.fixture(scope="class")
def proxy_adapter(adapter_config):
    adapter = LiveViewProxyAdapter(**adapter_config)
    return adapter


num_adapter_test_frames = 10


@pytest.fixture()
def adapter_test_frames():
    test_frames = []
    for i in range(num_adapter_test_frames):
        test_frames.append(Frame([json_encode({"frame_num": i}), "0"]))

    return test_frames


@pytest.fixture()
def proxy_request():
    request = Mock
    request.headers = {"Accept": "application/json", "Content-Type": "application/json"}
    return request


@pytest.fixture()
def frame_source():
    class FrameSource:
        def __init__(self):
            self.has_dropped_frame = False

        def dropped_frame(self):
            self.has_dropped_frame = True

    return FrameSource()


class TestLiveViewProxyAdapter:
    def test_adapter_default_init(self):
        default_adapter = LiveViewProxyAdapter(**{})

        assert default_adapter.dest_endpoint == DEFAULT_DEST_ENDPOINT
        assert default_adapter.drop_warn_percent == DEFAULT_DROP_WARN_PERCENT
        assert default_adapter.max_queue == DEFAULT_QUEUE_LENGTH
        assert default_adapter.source_endpoints[0].endpoint == DEFAULT_SOURCE_ENDPOINT

    def test_adapter_init(self, proxy_adapter, adapter_config):
        assert proxy_adapter.drop_warn_percent == adapter_config["dropped_frame_warning_cutoff"]
        assert proxy_adapter.dest_endpoint == adapter_config["destination_endpoint"]
        assert len(proxy_adapter.source_endpoints)
        for node in proxy_adapter.source_endpoints:
            assert "{}={}".format(node.name, node.endpoint) in adapter_config["source_endpoints"]

    def test_adapter_queue(self, proxy_adapter, adapter_test_frames):
        for frame in reversed(adapter_test_frames):
            proxy_adapter.add_to_queue(frame, self)

        returned_frames = []
        while not proxy_adapter.queue.empty():
            returned_frames.append(proxy_adapter.get_frame_from_queue())

        assert adapter_test_frames == returned_frames

    def test_adapter_already_connect_exception(self, adapter_config, caplog):
        # Second adapter instance binding to same endpoint
        second_adapter = LiveViewProxyAdapter(**adapter_config)
        assert log_message_seen(caplog, logging.ERROR, "Address already in use")

    def test_adapter_bad_config(self, adapter_config, caplog):
        bad_config = adapter_config
        bad_config["source_endpoints"] = "bad_node=not.a.real.socket"
        caplog.clear()
        LiveViewProxyAdapter(**bad_config)
        assert log_message_seen(caplog, logging.ERROR, "Error parsing target list")

        bad_config["source_endpoints"] = "not_even_parsable"
        caplog.clear()
        LiveViewProxyAdapter(**bad_config)
        assert log_message_seen(caplog, logging.ERROR, "Error parsing target list")

    def test_adapter_get(self, proxy_adapter, proxy_request, config_node_list):
        response = proxy_adapter.get("", proxy_request)

        assert response.status_code == 200
        assert "target_endpoint" in response.data
        assert len(response.data["nodes"]) == len(config_node_list)
        for key in response.data["nodes"]:
            assert key in config_node_list
            assert response.data["nodes"][key]["endpoint"] == config_node_list[key]

    def test_adapter_put_reset(self, proxy_adapter, proxy_request):
        proxy_request.body = '{"reset": 0}'
        proxy_adapter.last_sent_frame = (1, 2)
        proxy_adapter.dropped_frame_count = 5

        response = proxy_adapter.put("", proxy_request)
        assert "last_sent_frame" in response.data
        assert response.data["last_sent_frame"] == (0, 0)
        assert response.data["dropped_frames"] == 0

    def test_adapter_put_invalid(self, proxy_adapter, proxy_request):
        proxy_request.body = '{"invalid": "Invalid"}'
        response = proxy_adapter.put("", proxy_request)

        assert "error" in response.data
        assert response.status_code == 400

    def test_adapter_fill_queue(self, adapter_config, proxy_adapter, frame_source):
        frame_list = []
        queue_length = adapter_config["queue_length"]
        for i in range(queue_length + 1):
            frame_list.append(Frame([json_encode({"frame_num": i}), "0"]))

        for frame in frame_list[:queue_length]:
            proxy_adapter.add_to_queue(frame, frame_source)
        assert proxy_adapter.dropped_frame_count == 0

        proxy_adapter.add_to_queue(frame_list[-1], self)
        assert proxy_adapter.dropped_frame_count == 1

        result_frames = []
        while not proxy_adapter.queue.empty():
            result_frames.append(proxy_adapter.queue.get_nowait())
        assert frame_list[1:] == result_frames

    def test_adapter_drop_old_frame(self, proxy_adapter, frame_source):
        frame_new = Frame([json_encode({"frame_num": 5}), "0"])
        frame_old = Frame([json_encode({"frame_num": 1}), "0"])

        proxy_adapter.add_to_queue(frame_new, frame_source)
        get_frame = proxy_adapter.get_frame_from_queue()
        assert frame_new == get_frame
        assert proxy_adapter.last_sent_frame == (0, 5)

        proxy_adapter.add_to_queue(frame_old, frame_source)
        assert frame_source.has_dropped_frame
        assert proxy_adapter.queue.empty()
