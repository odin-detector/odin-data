import pytest
from odin_data.util import (
    cast_bytes,
    cast_unicode,
    construct_version_dict,
    remove_prefix,
    remove_suffix,
)


class TestUtils:
    def test_remove_prefix(self):
        test = "config/hdf/frames"
        expected = "hdf/frames"

        result = remove_prefix(test, "config/")

        assert expected == result

    def test_remove_suffix(self):
        test = "hdf/frames/0"
        expected = "hdf/frames"

        result = remove_suffix(test, "/0")

        assert expected == result

    def test_cast_bytes(self):
        test = b"string"
        expected = b"string"

        result = cast_bytes(test)

        assert result == expected

        test = "string"
        result = cast_bytes(test)

        assert result == expected

        test = 1.0
        with pytest.raises(TypeError) as excinfo:
            result = cast_bytes(test)

        assert excinfo.type is TypeError

    def test_cast_unicode(self):
        test = "string"
        expected = "string"

        result = cast_unicode(test)

        assert result == expected

        test = b"string"
        result = cast_unicode(test)

        assert result == expected

        test = 1.0
        with pytest.raises(TypeError) as excinfo:
            result = cast_unicode(test)

        assert excinfo.type is TypeError

    def test_construct_version_dict(self):
        test = "1.2.3dev4"
        result = construct_version_dict(test)
        assert result["major"] == "1"
        assert result["minor"] == "2"
        assert result["patch"] == "3"
        assert result["short"] == "1.2.3"
