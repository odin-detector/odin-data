import json
import datetime
import sys

'''Check the python version at runtime. DECODE_BYTES is True when running on python 3.0 - 3.5'''
if sys.version_info[0] == 3:
    if sys.version_info[1] <= 5:
        DECODE_BYTES = True

class IpcMessageException(Exception):
    def __init__(self, msg, errno=None):
        self.msg = msg
        self.errno = errno

    def __str__(self):
        return str(self.msg)


class IpcMessage(object):

    ACK = "ack"
    NACK = "nack"

    def __init__(self, msg_type=None, msg_val=None, from_str=None):
        self.attrs = {}

        if from_str is None:
            self.attrs['msg_type'] = msg_type
            self.attrs['msg_val'] = msg_val
            self.attrs['timestamp'] = datetime.datetime.now().isoformat()
            self.attrs['params'] = {}
        else:
            try:
                '''Manually decode bytes when operating in python versions 3.0 - 3.5 inclusive'''
                if DECODE_BYTES:
                    from_str = from_str.decode("utf-8")
                self.attrs = json.loads(from_str)
            except ValueError as e:
                raise IpcMessageException(
                    "Illegal message JSON format: " + str(e))

    def is_valid(self):
        is_valid = True
        try:
            is_valid = is_valid & (self._get_attr("msg_type") is not None)
            is_valid = is_valid & (self._get_attr("msg_val") is not None)
            is_valid = is_valid & (self._get_attr("timestamp") is not None)
        except IpcMessageException:
            is_valid = False

        return is_valid

    def get_msg_type(self):
        return self.attrs['msg_type']

    def get_msg_val(self):
        return self.attrs['msg_val']

    def get_msg_timestamp(self):
        return self.attrs['timestamp']

    def get_param(self, param_name, default_value=None):
        try:
            param_value = self.attrs['params'][param_name]
        except KeyError:
            if default_value is None:
                raise IpcMessageException("Missing parameter " + param_name)
            else:
                param_value = default_value

        return param_value

    def set_msg_type(self, msg_type):
        self.attrs['msg_type'] = msg_type

    def set_msg_val(self, msg_val):
        self.attrs['msg_val'] = msg_val

    def set_param(self, param_name, param_value):
        if "params" not in self.attrs:
            self.attrs['params'] = {}

        self.attrs['params'][param_name] = param_value

    def encode(self):
        return json.dumps(self.attrs)

    def __eq__(self, other):
        return self.attrs == other.attrs

    def __ne__(self, other):
        return self.attrs != other.attrs

    def __str__(self):
        return json.dumps(self.attrs,
                          sort_keys=True, indent=4, separators=(',', ': '))

    def _get_attr(self, attr_name, default_value=None):

        try:
            attr_value = self.attrs[attr_name]
        except KeyError:
            if default_value is None:
                raise IpcMessageException("Missing attribute " + attr_name)
            else:
                attr_value = default_value

        return attr_value
