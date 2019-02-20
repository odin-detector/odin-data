import sys
import threading
import logging
import tornado.gen

if sys.version_info[0] == 3:  # pragma: no cover
    from unittest.mock import Mock, patch
else:                         # pragma: no cover
    from mock import Mock, patch

from nose.tools import assert_equals, assert_raises, assert_true, assert_false,\
    assert_equal, assert_not_equal

from tornado.escape import json_decode, json_encode

from odin_data.live_view_proxy_adapter import LiveViewProxyAdapter, LiveViewProxyNode, Frame


class TestFrame():
    
    @classmethod
    def setup_class(cls):
        cls.frame_1_header = {"frame_num": 1}
        cls.frame_1 = Frame([json_encode({"frame_num": 1}), 0])
        cls.frame_2 = Frame([json_encode({"frame_num": 2}), 0])

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


class TestProxyNode():

    @classmethod
    def callback(cls):
        cls.callback_count += 1

    @classmethod
    def setup_class(cls):
        cls.callback_count = 0
        cls.node_1_info = {"name": "test_node_1", "endpoint": "tcp://127.0.0.1:5010"}
        cls.node_1 = LiveViewProxyNode(
            cls.node_1_info["name"],
            cls.node_1_info["endpoint"],
            0.5,
            cls.callback)

    @classmethod
    def teardown_class(cls):
        cls.callback_count = 0

    def test_node_init(self):
        assert_equal(self.node_1.name, self.node_1_info["name"])


