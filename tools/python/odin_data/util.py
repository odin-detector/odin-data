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
