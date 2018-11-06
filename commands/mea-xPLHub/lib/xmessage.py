# Copyright (c) 2009-2012 Simon Kennedy <code@sffjunkie.co.uk>.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#from expel.message.xaddress import xAddress
from xaddress import xAddress

__all__ = ['xMessage', 'xMessageBlock']

class xMessage(object):
    """A base class for handling both xPL and xAP messages"""
    
    def __init__(self, max_line_length=-1, split_char=','):
        self._version = ''
        self._hop = 1
        self._source_vendor = ''
        self._source_device = ''
        self._source_instance = ''
        self._target_vendor = '*'
        self._target_device = '*'
        self._target_instance = '*'
        self._wildcard_target = True
        self._blocks = []
        self._is_received = False
        self._is_sent = False
        
        # Default to xPL format addresses
        self.mode = 'xpl'

        self._max_line_length = max_line_length
        self._split_char = split_char

    def version():
        doc = "The application\'s version number."
        
        def fget(self):
            return self._version

        def fset(self, value):
            self._version = value
            
        return locals()

    version = property(**version())

    def hop():
        doc = """The hop count.
        
        Each time an xPLMessage is transferred between 2 different transport
        methods e.g. Ethernet to serial, the hop count is increased. If the hop
        count goes above a set limit the message should not be re-sent as it
        indicates a message bouncing between the 2 transports. 
        """
        
        def fget(self):
            return self._hop
    
        def fset(self, value):
            self._hop = int(value)
            
        return locals()

    hop = property(**hop())

    def source_vendor():
        doc = "The vendor id of the source of the message"
        
        def fget(self):
            return self._source_vendor
    
        def fset(self, value):
            v = str(value).lower()
            result = _check_string(v, 1, 8, False)
            if result != '':
                raise ValueError(result)
            else:
                self._source_vendor = v

        return locals()

    source_vendor = property(**source_vendor())

    def source_device():
        doc = "The device id of the source of the message"
        
        def fget(self):
            return self._source_device
    
        def fset(self, value):
            v = str(value).lower()
            result = _check_string(v, 1, 8, False)
            if result != '':
                raise ValueError(result)
            else:
                self._source_device = v

        return locals()

    source_device = property(**source_device())

    def source_instance():
        doc = "The instance id of the source of the message"
        
        def fget(self):
            return self._source_instance
    
        def fset(self, value):
            v = str(value).lower()
            result = _check_string(v, 1, 16, False)
            if result != '':
                raise ValueError(result)
            else:
                self._source_instance = v

        return locals()

    source_instance = property(**source_instance())

    def source():
        doc = """The source address as an
        :class:`~expel.message.xmessage.xAddress` instance."""
        
        def fget(self):
            return xAddress((self.source_vendor,
                self.source_device,
                self.source_instance),
                mode=self.mode)
            
        def fset(self, value):
            self.source_vendor, self.source_device, self.source_instance = \
                value[:3]

        return locals()

    source = property(**source())

    def target_vendor():
        doc = "The vendor id of the target of the message"
        
        def fget(self):
            if self._wildcard_target:
                return '*'
            else:
                return self._target_vendor
    
        def fset(self, value):
            v = str(value).lower()
            result = _check_string(v, 1, 8, False)
            if result != '':
                raise ValueError(result)
            else:
                self._target_vendor = v
                if v == '*' and self._target_device == '*' and \
                                self._target_instance == '*':
                    self._wildcard_target = True
                else:
                    self._wildcard_target = False

        return locals()

    target_vendor = property(**target_vendor())

    def target_device():
        doc = "The device id of the target of the message"
        
        def fget(self):
            if self._wildcard_target:
                return '*'
            else:
                return self._target_device
    
        def fset(self, value):
            v = str(value).lower()
            result = _check_string(v, 1, 8, False)
            if result != '':
                raise ValueError(result)
            else:
                self._target_device = v
                if v == '*' and self._target_vendor == '*' and \
                                self._target_instance == '*':
                    self._wildcard_target = True
                else:
                    self._wildcard_target = False

        return locals()

    target_device = property(**target_device())

    def target_instance():
        doc = "The instance id of the target of the message"
        
        def fget(self):
            if self._wildcard_target:
                return '*'
            else:
                return self._target_instance
    
        def fset(self, value):
            v = str(value).lower()
            result = _check_string(v, 1, 16, False)
            if result != '':
                raise ValueError(result)
            else:
                self._target_instance = v
                if v == '*' and self._target_vendor == '*' and \
                                self._target_device == '*':
                    self._wildcard_target = True
                else:
                    self._wildcard_target = False

        return locals()

    target_instance = property(**target_instance())

    def target():
        doc = """The target address as an
        :class:`~expel.message.xmessage.xAddress` instance."""
        
        def fget(self):
            if self._wildcard_target:
                return xAddress(('*', '*', '*'))
            else:
                return xAddress((self.target_vendor,
                    self.target_device,
                    self.target_instance),
                    format_string=self.mode)
            
        def fset(self, value):
            if isinstance(value, tuple):
                self.target_vendor, self.target_device, self.target_instance = \
                    value[:3]
                    
                if value == (('*', '*', '*')):
                    self._wildcard_target = True
            elif value == '*':
                self.target_vendor = self.target_device = \
                    self.target_instance = '*'
                self._wildcard_target = True

        return locals()

    target = property(**target())

    def wildcard_target():
        doc = "Is this message targeted at all targets"
        
        def fget(self):
            return self._wildcard_target
    
        def fset(self, value):
            if value:
                self._target_vendor = '*'
                self._target_device = '*'
                self._target_instance = '*'
    
            self._wildcard_target = value

        return locals()

    wildcard_target = property(**wildcard_target())

    def target_match(self, value):
        """Check if this message is targeted to this target"""  
        
        return self._wildcard_target or self.target == value

    def add_block(self):
        """Create an :class:`~expel.message.xmessage.xMessageBlock`,
        add it to our message and return the block"""
        
        block = xMessageBlock(self.max_line_length, self.split_char)
        self._blocks.append(block)
        return block

    blocks = property(lambda self: self._blocks)
        
    def max_line_length():
        def fget(self):
            return self._max_line_length
        
        def fset(self, value):
            self._max_line_length = value
            for block in self._blocks:
                block.max_line_length = value
                
        return locals()
    
    max_line_length = property(**max_line_length())
        
    def split_char():
        def fget(self):
            return self._split_char
        
        def fset(self, value):
            self._split_char = value
            for block in self._blocks:
                block.split_char = value
                
        return locals()
    
    split_char = property(**split_char())


class xMessageBlock(object):
    """:class:`xMessage` can contain any number of individual
    :class:`xMessageBlock`\s each with their own schema class and schema type.
    """
    
    def __init__(self, max_line_length=-1, split_char=','):
        self._schema_class = ''
        self._schema_type = ''
        self._data = {}
        self.max_line_length = max_line_length
        self.split_char = split_char

    def __getitem__(self, key):
        return self._data[key]

    def __setitem__(self, key, value):
        if isinstance(value, list):
            self._data[key] = value
        else:
            self._data[key] = [value]
            
    def __delitem__(self, key):
        del self._data[key]

    def get(self, key, default):
        return self._data.get(key, default)

    def scalar(self, key):
        return ','.join(self._data[key])

    def items(self):
        return self._data.items()
        
    def keys(self):
        return self._data.keys()
        
    def values(self):
        return self._data.values()
        
    def __iter__(self):
        return self._data.__iter__()

    def __len__(self):
        return len(self._data)

    def clear(self):
        self._data.clear()

    def schema_class():
        doc = "The schema class of this message block"
        
        def fget(self):
            return self._schema_class
    
        def fset(self, value):
            v = str(value)
            result = _check_string(v, 1, 8, True)
            if result != '':
                raise ValueError(result)
            else:
                self._schema_class = v

        return locals()

    schema_class = property(**schema_class())

    def schema_type():
        doc = "The schema type of this message block"
        
        def fget(self):
            return self._schema_type
    
        def fset(self, value):
            v = str(value)
            result = _check_string(v, 1, 8, True)
            if result != '':
                raise ValueError(result)
            else:
                self._schema_type = v

        return locals()

    schema_type = property(**schema_type())

    schema = property(lambda self: '%s.%s' % (self._schema_class,
                                              self._schema_type))

    def marshall(self, sort_keys=False):
        """Convert this :class:`xMessageBlock` into a string representation"""
        
        body = '%s\n{\n' % self.schema

        keys = self._data.keys()
        if sort_keys:
            keys = sorted(keys)
            
        for key in keys:
            value = self._data[key]
            if isinstance(value, list):
                for item in value:
                    body += self._build_chunk(key, item)
            else:
                body += self._build_chunk(key, value)

        return body + '}\n'

    def _build_chunk(self, key, value):
        value = str(value)
        chunk = ''
        
        # If we specify a max line length attempt to split it up
        # using split_char
        line_length = len(key) + 1 + len(value)
        if self.max_line_length != -1 and line_length > self.max_line_length:
            if value.find(self.split_char) != -1:
                items = value.split(self.split_char)
                line = '%s=' % key
                for value in items:
                    if len(line) + len(value) > self.max_line_length:
                        chunk += '%s\n' % line[:-1]
                        line = '%s=%s%s' % (key, value, self.split_char)
                    else:
                        line = '%s%s%s' % (line, value, self.split_char)
                        
                chunk += '%s\n' % line[:-1]
            else:
                rest_of_line_length = self.max_line_length - (len(key) + 1)
                while len(value) > rest_of_line_length:
                    chunk += '%s=%s\n' % (key, value[:rest_of_line_length])
                    value = value[rest_of_line_length:]
                    
                chunk += '%s=%s\n' % (key, value)    
        else:
            chunk += '%s=%s\n' % (key, value)
            
        return chunk
            

def _check_string(theString, minLen, maxLen, lower):
    theString = str(theString)

    if len(theString) < minLen or len(theString) > maxLen:
        return '%s - Illegal field length (%d to %d)' % (theString, minLen,
                                                         maxLen)

    if lower and theString.lower() != theString:
        return '%s - Illegal casing' % theString

    return ''
