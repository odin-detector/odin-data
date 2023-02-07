import unittest

from odin_data.util import remove_prefix, remove_suffix


class UtilTest(unittest.TestCase):

    def test_remove_prefix(self):
        test = "config/hdf/frames"
        expected = "hdf/frames"

        result = remove_prefix(test, "config/")

        self.assertEqual(expected, result)

    def test_remove_suffix(self):
        test = "hdf/frames/0"
        expected = "hdf/frames"

        result = remove_suffix(test, "/0")

        self.assertEqual(expected, result)
