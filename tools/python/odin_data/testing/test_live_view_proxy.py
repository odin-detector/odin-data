import sys

from nose.tools import assert_equals, assert_true, assert_false,\
    assert_equal, assert_not_equal

from tornado.escape import json_encode
from odin_data.ipc_tornado_channel import IpcTornadoChannel

from odin_data.live_view_proxy_adapter import LiveViewProxyAdapter, LiveViewProxyNode, Frame, \
    DEFAULT_DEST_ENDPOINT, DEFAULT_DROP_WARN_PERCENT, DEFAULT_QUEUE_LENGTH, DEFAULT_SOURCE_ENDPOINT

from odin.testing.utils import OdinTestServer

from zmq.error import ZMQError

if sys.version_info[0] == 3:  # pragma: no cover
    from unittest.mock import Mock, patch
else:                         # pragma: no cover
    from mock import Mock, patch


class TestFrame():

    @classmethod
    def setup(self):
        self.frame_1_header = {"frame_num": 1}
        self.frame_1 = Frame([json_encode({"frame_num": 1}), 0])
        self.frame_2 = Frame([json_encode({"frame_num": 2}), 0])

    def test_frame_init(self):
        assert_equal(self.frame_1.get_number(), self.frame_1_header["frame_num"])

    def test_frame_get_header(self):
        assert_equal(self.frame_1.get_header(), json_encode(self.frame_1_header))

    def test_frame_get_data(self):
        assert_equal(self.frame_1.get_data(), 0)

    def test_frame_set_acq(self):
        self.frame_1.set_acq(5)
        assert_equal(self.frame_1.get_acq(), 5)

    def test_frame_order(self):
        assert_true(self.frame_1 < self.frame_2)

    def test_frame_order_acq(self):
        self.frame_1.set_acq(2)
        self.frame_2.set_acq(1)

        assert_true(self.frame_1 > self.frame_2)


class TestProxyNode(object):

    def callback(self, frame, source):
        self.callback_count += 1

    def setup(self):

        self.callback_count = 0
        self.node_1_info = {"name": "test_node_1", "endpoint": "tcp://127.0.0.1:5010"}
        self.node_2_info = {"name": "test_node_2", "endpoint": "tcp://127.0.0.1:5011"}
        self.node_1 = LiveViewProxyNode(
            self.node_1_info["name"],
            self.node_1_info["endpoint"],
            0.5,
            self.callback)
        self.node_2 = LiveViewProxyNode(
            self.node_2_info["name"],
            self.node_2_info["endpoint"],
            0.5,
            self.callback)

        self.test_frame = Frame([json_encode({"frame_num": 1}), "0"])

    @classmethod
    def teardown(self):
        self.callback_count = 0

    def test_node_init(self):
        assert_equal(self.node_1.get_name(), self.node_1_info["name"])
        assert_equal(self.node_1.get_endpoint(), self.node_1_info["endpoint"])

    def test_node_reset(self):
        self.node_1.received_frame_count = 10
        self.node_1.dropped_frame_count = 5
        self.node_1.has_warned = True
        self.node_1.set_reset()
        assert_equal(self.node_1.received_frame_count, 0)
        assert_equal(self.node_1.dropped_frame_count, 0)
        assert_false(self.node_1.has_warned)

    def test_node_dropped_frame(self):
        self.node_1.received_frame_count = 10
        for _ in range(9):
            self.node_1.dropped_frame()

        assert_equal(self.node_1.get_dropped_frames(), 9)
        assert_true(self.node_1.has_warned)

    def test_node_callback(self):
        test_header = {"frame_num": 1}
        for i in range(10):
            test_header["frame_num"] = i
            self.node_1.local_callback([json_encode(test_header), 0])
        test_header["frame_num"] = 1
        self.node_1.local_callback([json_encode(test_header), 0])
        assert_equal(self.node_1.get_current_acq(), 1)
        assert_equal(self.node_1.get_received_frames(), 11)
        assert_equal(self.node_1.get_last_frame(), 1)


class TestLiveViewProxyAdapter(OdinTestServer):

    def setup(self):
        self.config_node_list = {
            "test_node_1": "tcp://127.0.0.1:5000",
            "test_node_2": "tcp://127.0.0.1:5001"
        }
        self.adapter_config = {
            "destination_endpoint": "tcp://127.0.0.1:5021",
            "source_endpoints":
                "\n{}={},".format(self.config_node_list.keys()[0], self.config_node_list.values()[0]) +
                "\n{}={}".format(self.config_node_list.keys()[1], self.config_node_list.values()[1]),
            "dropped_frame_warning_cutoff": 0.2,
            "queue_length": 15
        }
        # access_logging = 'debug'
        # super(TestLiveViewProxyAdapter, self).setup_cTestLiveViewProxyAdapterlass(self.adapter_config, access_logging)
        self.test_frames = []
        for i in range(10):
            tmp_frame = Frame([json_encode({"frame_num": i}), "0"])
            self.test_frames.append(tmp_frame)
        self.adapter = LiveViewProxyAdapter(**self.adapter_config)

        self.request = Mock
        self.request.headers = {'Accept': 'application/json', 'Content-Type': 'application/json'}
        self.path = ""
        self.has_dropped_frame = False

    def teardown(self):
        super(TestLiveViewProxyAdapter, self).teardown_class()
        self.adapter.cleanup()

    def dropped_frame(self):
        self.has_dropped_frame = True

    def test_adapter_init(self):
        assert_equal(
            self.adapter.drop_warn_percent,
            self.adapter_config["dropped_frame_warning_cutoff"]
            )
        assert_equal(self.adapter.dest_endpoint, self.adapter_config["destination_endpoint"])
        assert_true(self.adapter.source_endpoints)
        for node in self.adapter.source_endpoints:
            assert_true("{}={}".format(node.get_name(), node.get_endpoint()) in self.adapter_config["source_endpoints"])

    def test_adapter_default_init(self):
        default_adapter = LiveViewProxyAdapter(**{})

        assert_equal(default_adapter.dest_endpoint, DEFAULT_DEST_ENDPOINT)
        assert_equal(default_adapter.drop_warn_percent, DEFAULT_DROP_WARN_PERCENT)
        assert_equal(default_adapter.max_queue, DEFAULT_QUEUE_LENGTH)
        assert_equal(default_adapter.source_endpoints[0].get_endpoint(), DEFAULT_SOURCE_ENDPOINT)
    
    def test_adapter_queue(self):
        for frame in reversed(self.test_frames):
            self.adapter.add_to_queue(frame, self)

        returned_frames = []
        while not self.adapter.queue.empty():
            returned_frames.append(self.adapter.get_frame_from_queue())

        assert_equals(self.test_frames, returned_frames)

    def test_adapter_already_connect_exception(self):
        try:
            # second adapter instance binding to same endpoint
            LiveViewProxyAdapter(**self.adapter_config)
        except ZMQError:
            assert_false("Live View raised ZMQError when binding second socket to same address.")

    def test_adapter_bad_config(self):
        bad_config = self.adapter_config
        bad_config["source_endpoints"] = "bad_node=not.a.real.socket"
        LiveViewProxyAdapter(**bad_config)
        bad_config["source_endpoints"] = "not_even_parsable"
        LiveViewProxyAdapter(**bad_config)

    def test_adapter_get(self):
        response = self.adapter.get(self.path, self.request)
        assert_equal(response.status_code, 200)
        assert_true("target_endpoint" in response.data)
        assert_equal(len(response.data["nodes"]), len(self.config_node_list))
        for key in response.data["nodes"]:
            assert_true(key in self.config_node_list)
            assert_equal(response.data["nodes"][key]["endpoint"], self.config_node_list[key])

    def test_adapter_put_reset(self):
        self.request.body = '{"reset": 0}'
        self.adapter.last_sent_frame = (1, 2)
        self.adapter.dropped_frame_count = 5

        response = self.adapter.put(self.path, self.request)
        assert_true("last_sent_frame" in response.data)
        assert_equal(response.data["last_sent_frame"], (0, 0))
        assert_equal(response.data["dropped_frames"], 0)

    def test_adapter_put_invalid(self):
        self.request.body = '{"invalid": "Invalid"}'
        response = self.adapter.put(self.path, self.request)

        assert_true("error" in response.data)
        assert_equal(response.status_code, 400)

    def test_adapter_fill_queue(self):
        frame_list = []
        queue_length = self.adapter_config["queue_length"]
        for i in range(queue_length + 1):
            tmp_frame = Frame([json_encode({"frame_num": i}), "0"])
            frame_list.append(tmp_frame)
        for frame in frame_list[:queue_length]:
            self.adapter.add_to_queue(frame, self)
        assert_equal(self.adapter.dropped_frame_count, 0)
        self.adapter.add_to_queue(frame_list[-1], self)
        assert_equal(self.adapter.dropped_frame_count, 1)
        result_frames = []
        while not self.adapter.queue.empty():
            result_frames.append(self.adapter.queue.get_nowait())
        assert_equal(frame_list[1:], result_frames)

    def test_adapter_drop_old_frame(self):
        frame_new = Frame([json_encode({"frame_num": 5}), "0"])
        frame_old = Frame([json_encode({"frame_num": 1}), "0"])
        self.adapter.add_to_queue(frame_new, self)
        get_frame = self.adapter.get_frame_from_queue()
        assert_equal(frame_new, get_frame)
        assert_equal(self.adapter.get_last_frame(), (0, 5))
        self.adapter.add_to_queue(frame_old, self)
        assert_true(self.has_dropped_frame)
        assert_true(self.adapter.queue.empty())       
