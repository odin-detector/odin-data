import re
import sys

if sys.version_info[0] >= 3:
    unicode = str
    bytes = bytes
else:
    unicode = unicode
    bytes = str


def remove_prefix(string, prefix):
    return re.sub("^{}".format(prefix), "", string)


def remove_suffix(string, suffix):
    return re.sub("{}$".format(suffix), "", string)


def cast_bytes(s, encoding="utf8", errors="strict"):
    """Cast unicode or bytes to bytes

    :param obj: the dictionary, list, or unicode string
    :param encoding: encoding to use
    :param errors: error handling scheme

    :return: the same data type as obj, but with unicode strings converted to python
        strings.

    :raises: TypeError if `s` is not `unicode` or `bytes`
    """
    if isinstance(s, bytes):
        return s
    elif isinstance(s, unicode):
        return s.encode(encoding, errors)
    else:
        raise TypeError("Expected unicode or bytes, got '%r'" % s)


def cast_unicode(s, encoding="utf8", errors="strict"):
    """Cast bytes or unicode to unicode

    :param s: string to cast
    :param encoding: encoding to use
    :param errors: error handling scheme

    :return: unicode string

    :raises: TypeError if `s` is not `unicode` or `bytes`
    """
    if isinstance(s, unicode):
        return s
    elif isinstance(s, bytes):
        return s.decode(encoding, errors)
    else:
        raise TypeError("Expected unicode or bytes, got '%r'" % s)


MAJOR_VER_REGEX = r"^([0-9]+)[\\.-].*|$"
MINOR_VER_REGEX = r"^[0-9]+[\\.-]([0-9]+).*|$"
PATCH_VER_REGEX = r"^[0-9]+[\\.-][0-9]+[\\.-]([0-9]+).|$"


def construct_version_dict(full_version):
    major_version = re.findall(MAJOR_VER_REGEX, full_version)[0]
    minor_version = re.findall(MINOR_VER_REGEX, full_version)[0]
    patch_version = re.findall(PATCH_VER_REGEX, full_version)[0]
    short_version = major_version + "." + minor_version + "." + patch_version

    return dict(
        full=full_version,
        major=major_version,
        minor=minor_version,
        patch=patch_version,
        short=short_version,
    )
