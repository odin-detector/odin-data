import re


def remove_prefix(string, prefix):
    return re.sub("^{}".format(prefix), "", string)


def remove_suffix(string, suffix):
    return re.sub("{}$".format(suffix), "", string)


def convert_unicode_to_string(self, obj):
    """
    Convert all unicode parts of a dictionary or list to standard strings.

    This method may not handle special characters well!
    :param obj: the dictionary, list, or unicode string
    :return: the same data type as obj, but with unicode strings converted to python strings.
    """
    if isinstance(obj, dict):
        return {self.convert_unicode_to_string(key): self.convert_unicode_to_string(value)
                for key, value in obj.items()}
    elif isinstance(obj, list):
        return [self.convert_unicode_to_string(element) for element in obj]
    elif isinstance(obj, unicode):
        return obj.encode('utf-8')

    return obj


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
