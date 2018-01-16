import re


def remove_prefix(string, prefix):
    return re.sub("^{}".format(prefix), "", string)


def remove_suffix(string, suffix):
    return re.sub("{}$".format(suffix), "", string)
