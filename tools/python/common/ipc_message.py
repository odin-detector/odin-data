import json
import datetime

class IpcMessageException(Exception):
    
    def __init__(self, msg, errno=None):
        self.msg = msg
        self.errno = errno
        
    def __str__(self):
        return str(self.msg)
    
class IpcMessage(object):
    
    def __init__(self, msg_type=None, msg_val=None, from_str=None):
        
        self.attrs = {}
        
        if from_str == None:
            self.attrs['msg_type'] = msg_type
            self.attrs['msg_val']  = msg_val
            self.attrs['timestamp'] = datetime.datetime.now().isoformat()
            self.attrs['params'] = {}
            
        else:
            try:
                self.attrs = json.loads(from_str)
                
            except ValueError, e:
                raise IpcMessageException("Illegal message JSON format: " + str(e))
            
    def is_valid(self):
        
        is_valid = True
        
        try:
            is_valid = is_valid & (self._get_attr('msg_type') != None)
            is_valid = is_valid & (self._get_attr("msg_val") != None)
            is_valid = is_valid & (self._get_attr("timestamp") != None)
            
        except IpcMessageException, e:
            is_valid = False
            
        return is_valid            
            
    def get_msg_type(self, default_value=None):
        
        return self.attrs['msg_type']

    def get_msg_val(self, default_value=None):
        
        return self.attrs['msg_val']
    
    def get_msg_timestamp(self, default_val=None):
        
        return self.attrs['timestamp']
                          
    def get_param(self, param_name, default_value=None):
        
        try:
            param_value = self.attrs['params'][param_name]
            
        except KeyError, e:
            if default_value == None:
                raise IpcMessageException("Missing parameter " + param_name)
            else:
                param_value = default_value
                
        return param_value
    
    def set_msg_type(self, msg_type):
        
        self.attrs['msg_type'] = msg_type
        
    def set_msg_val(self, msg_val):
        
        self.attrs['msg_val'] = msg_val
        
    def set_param(self, param_name, param_value):
        
        if not 'params' in self.attrs:
            self.attrs['params'] = {}
            
        self.attrs['params'][param_name] = param_value
        
    def encode(self):
        
        return json.dumps(self.attrs)
    
    def __eq__(self, other):
        
        return self.attrs == other.attrs
    
    def __ne__(self, other):
        
        return self.attrs != other.attrs
    
    def __str__(self):
        
        return json.dumps(self.attrs, sort_keys=True, indent=4, separators=(',', ': '))
    
    def _get_attr(self, attr_name, default_value=None):
        
        try:
            attr_value = self.attrs[attr_name]
        
        except KeyError, e:
            if default_value == None:
                raise IpcMessageException("Missing attribute " + attr_name)
            else:
                attr_value = default_value
                
        return attr_value
