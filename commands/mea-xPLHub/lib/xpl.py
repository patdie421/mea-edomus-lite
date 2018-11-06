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
#from expel.message.xmessage import xMessage
from xmessage import xMessage
#from expel.message.xmessageparser import xMessageParser
from xmessageparser import xMessageParser

__all__ = ['xPLMessage',
    'xPLHBeatBasic', 'xPLHBeatApp',
    'xPLHBeatRequest', 'xPLHBeatEnd',
    'xPLCommand', 'xPLStatus', 'xPLTrigger',
    'xPLFragment', 'xPLFragmentManager',
    'xPLMessageError', 'xPLFragmentError']

XPL_HOP_WARNING = 128
XPL_MAX_MESSAGE_SIZE = 1472

class xPLMessageError(Exception):
    pass


class xPLFragmentError(xPLMessageError):
    pass


class xPLMessage(xMessage):
    """:class:`xPLMessage` stores information about an XPL message.
    
    It provides methods to...
    
    * Access all header and body items 
    * Convert to and from raw text
    * Return information about the message e.g. is it a heartbeat message,
      does it need fragmenting etc.
    """
    
    XPL_HDR_RAW  = '\n{\nhop=1\nsource=%s\ntarget=*\n}\n'
    XPL_CMND_RAW = 'xpl-cmnd' + XPL_HDR_RAW
    XPL_STAT_RAW = 'xpl-stat' + XPL_HDR_RAW
    XPL_TRIG_RAW = 'xpl-trig' + XPL_HDR_RAW

    HBEAT_BASIC_RAW  = XPL_STAT_RAW + 'hbeat.basic\n{\ninterval=%d\n}\n'
    HBEAT_APP_RAW    = XPL_STAT_RAW + ('hbeat.app\n{\ninterval=%d\n'
                                       'port=%d\nremote-ip=%s\n}\n')
    HBEAT_APPVER_RAW = XPL_STAT_RAW + ('hbeat.app\n{\ninterval=%d\n'
                                       'port=%d\nremote-ip=%s\nversion=%s\n}\n')
    HBEAT_END_RAW    = XPL_STAT_RAW + ('hbeat.end\n{\ninterval=%d\nport=%d\n'
                                       'remote-ip=%s\n}\n')
    HBEAT_REQ_RAW    = XPL_CMND_RAW + 'hbeat.request\n{\ncommand=request\n}\n'

    CONFIG_BASIC_RAW    = XPL_STAT_RAW + 'config.basic\n{\ninterval=%d\n}\n'
    CONFIG_APP_RAW      = XPL_STAT_RAW + ('config.app\n{\ninterval=%d\n'
                                          'port=%d\nremote-ip=%s\n}\n')
    CONFIG_LIST_RAW     = XPL_STAT_RAW + 'config.list\n{\n%s}\n'
    CONFIG_CURRENT_RAW  = XPL_STAT_RAW + 'config.current\n{\n%s}\n'
    
    XPL_FRAG_REQUEST_RAW = XPL_CMND_RAW + ('fragment.request\n{\n'
                                           'command=resend\n}\n') 

    def __init__(self, raw='', max_line_length=-1, split_char=','):
        """Create a new xPLMessage
        
        :param raw:             A string of data to use to construct the intial
                                message
        :type raw:              string
        :param max_line_length: The maximum length of lines when marshalling
                                the message. Lines longer than this will be
                                split into multiple items at instances of the
                                `split_char`
        :type max_line_length:  integer
        :param split_char':     The character to split long lines ',' by default
        :type split_char:       string
        """
        
        xMessage.__init__(self, max_line_length=max_line_length, split_char=split_char)

        self._type = 'cmnd'
        
        self.address_format = '%s-%s.%s'

        # xPL messages only have a single block
        self._block0 = self.add_block()

        if raw != '':
            self.unmarshall(raw)

    def __str__(self):
        return self.marshall(sort_keys=True)

    def __repr__(self):
        return self.marshall(sort_keys=True).replace('\n', ' ')

    def __iter__(self):
        return self._block0.__iter__()

    def items(self):
        return self._block0.items()

    def keys(self):
        return self._block0.keys()

    def values(self):
        return self._block0.values()
    
    def get(self, key, default):
        try:
            return self._block0[key]
        except KeyError:
            return default

    def __getitem__(self, key):
        item = self._block0[key]
        if len(item) > 1:
            return item
        else:
            return item[0]

    def __setitem__(self, key, value):
        return self._block0.__setitem__(key, value)
    
    def __delitem__(self, key):
        self._block0.__delitem__(key)

    def message_type():
        doc = """The message type - One of `cmnd`, `stat` or `trig`"""
        
        def fget(self):
            return self._type
            
        def fset(self, value):
            v = str(value).lower()
            mtypes = ['cmnd', 'stat', 'trig']
            if v not in mtypes:
                raise ValueError('message type must be one of %s' %
                                 ', '.join(mtypes))
            else:
                self._type = v

        return locals()
        
    message_type = property(**message_type())

    def schema_class():
        doc = """The class of the message's schema e.g. the `schema_class`
        of 'audio.basic' is 'audio'"""

        def fget(self):
            return self._block0.schema_class
    
        def fset(self, value):
            v = str(value).lower()
            self._block0.schema_class = v

        return locals()

    schema_class = property(**schema_class())

    def schema_type():
        doc = """The type of the message's schema e.g. the `schema_type` of
        'datetime.basic' is 'basic'"""

        def fget(self):
            return self._block0.schema_type
    
        def fset(self, value):
            v = str(value).lower()
            self._block0.schema_type = v

        return locals()

    schema_type = property(**schema_type())

    schema = property(lambda self: self._block0.schema)

    def is_valid():
        doc = ('Returns :keyword:`True` if the object has all the information '
            'necessary to emit a raw version of the message')
        
        def fget(self):
            svOk = len(self._source_vendor) > 0 and \
                len(self._source_vendor) <= 8
            sdOk = len(self._source_device) > 0 and \
                len(self._source_device) <= 8
            siOk = len(self._source_instance) > 0 and \
                len(self._source_instance) <= 16
    
            tvOk = len(self._target_vendor) > 0 and \
                len(self._target_vendor) <= 8
            tdOk = len(self._target_device) > 0 and \
                len(self._target_device) <= 8
            tiOk = len(self._target_instance) > 0 and \
                len(self._target_instance) <= 16
    
            scOk = len(self.schema_class) > 0 and len(self.schema_class) <= 8
            stOk = len(self.schema_type) > 0 and len(self.schema_type) <= 8
    
            if svOk and sdOk and siOk \
              and (self._wildcard_target or (tvOk and tdOk and tiOk)) and \
                   scOk and stOk:
                return True
            else:
                return False

        return locals()

    is_valid = property(**is_valid())
    
    def needs_fragmenting():
        doc = ('Returns :keyword:`True` if the message is too large for a '
        'standard message and needs fragmenting.')
        
        def fget(self):
            return len(self.marshall()) > XPL_MAX_MESSAGE_SIZE
        
        return locals()
    
    needs_fragmenting = property(**needs_fragmenting())
    
    def raw_header(self):
        """Return the string representation of this messages header."""
        
        return 'xpl-%s\n{\nhop=1\nsource=%s\ntarget=%s\n}\n' % \
            (self.message_type, self.source, self.target)

    def marshall(self, sort_keys=False):
        """Convert this :class:`xPLMessage` into its string representation.
        
        :param sort_keys:   If :keyword:`True` then the string representation
                            will output the message using its keys in a sorted
                            order.
        :type sort_keys:    boolean
        """
        
        if not self.is_valid:
            raise xPLMessageError(('Unable to construct valid raw xPL '
                                       'from supplied fields.'))
        hdr = 'xpl-%s\n{\nhop=1\nsource=%s\ntarget=%s\n}\n' % \
            (self.message_type, self.source, self.target)

        body = self._block0.marshall(sort_keys)

        return hdr + body

    def unmarshall(self, raw):
        """Extract an :class:`xPLMessage` from a string representation.
        
        :param raw:     The string representation of the data to extract.
        :type raw:      string
        """
        
        try:
            parser = xMessageParser('xpl')
            parser.parse(raw, self)
        except:
            raise xPLMessageError(('Unable to construct valid xPL message '
                                   'from raw string'))

    is_heartbeat = property(lambda self: (self.schema_class == 'hbeat'))
    is_heartbeat_app = \
        property(lambda self: (self.schema_class == 'hbeat') \
        and (self.schema_type == 'app'))
    is_heartbeat_basic = \
        property(lambda self: (self.schema_class == 'hbeat') and \
        (self.schema_type == 'basic'))
    is_heartbeat_end = \
        property(lambda self: (self.schema_class == 'hbeat') and \
        (self.schema_type == 'end'))


class xPLHBeatBasic(xPLMessage):
    """A basic xPL heartbeat message."""
    
    def __init__(self, source, interval=5):
        if isinstance(source, xAddress):
            source = str(source)
            
        xPLMessage.__init__(self, raw=xPLMessage.HBEAT_BASIC_RAW % (source,
            interval))


class xPLHBeatApp(xPLMessage):
    """An xPL heartbeat message with the address and port."""
    
    def __init__(self, source, address, port, interval=5):
        if isinstance(source, xAddress):
            source = str(source)
            
        xPLMessage.__init__(self, raw=xPLMessage.HBEAT_APP_RAW % (source,
            interval, port, address))


class xPLHBeatRequest(xPLMessage):
    """An xPL message used to request a heartbeat message."""
    
    def __init__(self, source):
        if isinstance(source, xAddress):
            source = str(source)
            
        xPLMessage.__init__(self, raw=xPLMessage.HBEAT_REQ_RAW % source)


class xPLHBeatEnd(xPLMessage):
    """An xPL heartbeat end message sent when an xPL endpoint closes."""
    
    def __init__(self, source, address, port, interval=5):
        if isinstance(source, xAddress):
            source = str(source)
            
        xPLMessage.__init__(self, raw=xPLMessage.HBEAT_END_RAW % (source,
            interval, port, address))


class xPLCommand(xPLMessage):
    """An empty xPL command message."""
    
    def __init__(self, source):
        if isinstance(source, xAddress):
            source = str(source)
            
        xPLMessage.__init__(self, raw=xPLMessage.XPL_CMND_RAW % source)


class xPLStatus(xPLMessage):
    """An empty xPL status message."""
    
    def __init__(self, source):
        if isinstance(source, xAddress):
            source = str(source)
            
        xPLMessage.__init__(self, raw=xPLMessage.XPL_STAT_RAW % source)


class xPLTrigger(xPLMessage):
    """An empty xPL trigger message."""
    
    def __init__(self, source):
        if isinstance(source, xAddress):
            source = str(source)
            
        xPLMessage.__init__(self, raw=xPLMessage.XPL_TRIG_RAW % source)


class xPLFragment(xPLMessage):
    """An :class:`xPLFragment` is used when the data in an
    :class:`~expel.message.xpl.xPLMessage` would be too large to
    send as a single packet of information over UDP
    """
    
    def __init__(self, for_message=None):
        """Create a new fragment
        
        :param for_message: If specified then use this message to construct the
                            fields of the fragment
        :type for_message:  :class:`~expel.message.xpl.xPLMessage`
        """
        
        xPLMessage.__init__(self)
        self.schema_class = 'fragment'
        self.schema_type = 'basic'
        self['partid'] = ''
        self['schema'] = ''
        self._number = -1
        self._count = -1
        self._message_id = ''
        
        if for_message is not None:
            self.source = for_message.source
            self.target = for_message.target
            self.hop = for_message.hop

    def number():
        doc = 'The position of this fragment within all the fragments'
        
        def fget(self):
            return self._number
        
        def fset(self, value):
            self._number = int(value)
            self['partid'] = '%d/%d:%s' % (self._number, self._count,
                                           self._message_id)
            
        return locals()
    
    number = property(**number())

    def count():
        doc = 'The number of fragments in the complete message'
        
        def fget(self):
            return self._count
        
        def fset(self, value):
            self._count = int(value)
            self['partid'] = '%d/%d:%s' % (self._number, self._count,
                                           self._message_id)
            
        return locals()
    
    count = property(**count())

    def message_id():
        doc = 'The message id of the fragments'
        
        def fget(self):
            return self._message_id
        
        def fset(self, value):
            self._message_id = value
            self['partid'] = '%d/%d:%s' % (self._number, self._count,
                                           self._message_id)
            
        return locals()
    
    message_id = property(**message_id())

    def part_id():
        doc = """The part_id of this fragment of the form
        :samp:`{number}/{count}:{message_id}`
        """
        
        def fget(self):
            return self['partid']
        
        def fset(self, value):
            number, rest = value.split('/')
            self._number = int(number)
            
            count, self._message_id = rest.split(':')
            self._count = int(count)
            self['partid'] = value
            
        return locals()
    
    part_id = property(**part_id())

    def message_schema():
        def fget(self):
            return self['schema']
        
        def fset(self, value):
            self['schema'] = value
            
        return locals()
    
    message_schema = property(**message_schema())


class xPLFragmentRequest(xPLMessage):
    """An :class:`~expel.message.xpl.xPLMessage` sent to request a missing
    part of a fragmented message
    """
    
    def __init__(self, source):
        xPLMessage.__init__(self, raw=xPLMessage.XPL_FRAG_REQUEST_RAW % source)


class xPLFragmentManager(object):
    """Assembles :class:`~expel.message.xpl.xPLFragment`\s into complete
    :class:`~expel.message.xpl.xPLMessage`\s and
    splits :class:`~expel.message.xpl.xPLMessage`\s into
    :class:`~expel.message.xpl.xPLFragment`\s.
    """
    
    def __init__(self):
        self.fragments = {}
        """Dictionary of information about the fragments received keyed by
        message id."""
        
        self.complete = {}
        """A dictionary of complete defragged messages keyed by message id"""
        
        self.id_generator = self._id_generator()
    
    def add(self, message):
        """Add an xPL Fragment to be merged
        
        :param message: The xPL fragment message to process
        :type message: :class:`~expel.message.xpl.xPLMessage`
        """
        
        if not isinstance(message, xPLFragment) and \
                message.schema != 'fragment.basic':
            raise ValueError('Message is not an xPL fragment')

        part = -1
        part_count = -1        
        message_id = -1
        
        try:
            partid = message['partid']
            
            part, rest = partid.split('/')
            part_count, message_id = rest.split(':')
            
            part = int(part)
            part_count = int(part_count)
        except KeyError:
            raise xPLFragmentError("No part_id specified")
        except ValueError:
            raise xPLFragmentError("Invalid part_id '%s' specified" % partid)

        if message_id in self.complete:
            return
            
        try:
            schema = message['schema']
        except KeyError:
            if part == 1:
                raise xPLFragmentError('Schema missing.')
            else:
                schema = ''
        
        if message_id not in self.fragments:
            info = [part_count, schema, part_count * [None]]
            self.fragments[message_id] = info
        else:
            info = self.fragments[message_id]
            
        if info is not None:
            if info[0] != part_count:
                raise xPLFragmentError(('xPL fragment for message %s '
                                        'received with '
                                        'different number of parts') % \
                                       message_id)
                
            if info[2][part-1] is not None:
                raise xPLFragmentError('Duplicate xPL fragment received')
            
            info[2][part-1] = message
            
            # If we have all the parts replace with a defragmented xPLMessage
            if len([x for x in info[2] if x is not None]) == part_count:
                msg = self._combine_fragments(info[1], info[2])
                self.complete[message_id] = msg

    def split(self, message, message_id=''):
        """Splits a message into a list of xPL fragments with the specified
        message id.
        
        :param message: The message to split
        :type message: :class:`~expel.message.xpl.xPLMessage`
        :param message_id: The message id to assign to the returned fragments
        :type message_id: string
        :rtype: :class:`~expel.message.xpl.xPLFragment`\[] or the message
                passed in if it does not need splitting into fragments 
        """
        
        if message_id == '':
            message_id = self.id_generator.next()
        
        if isinstance(message, xPLFragment):
            return message
        
        if not message.needs_fragmenting:
            return message
        
        header = message.raw_header()
        # available space for key, value pairs =
        # XPL_MAX_MESSAGE_SIZE - len(header+'fragment.basic'+'{\n'+'\n}\n')
        available = XPL_MAX_MESSAGE_SIZE - (len(header) + 19)
        
        fragments = []
        fragment = xPLFragment(message)
        
        count = 0
        for key, value in message.items():
            line_size = len('%s=%s\n' % (key, value))
            
            if count + line_size > available:
                fragments.append(fragment)
                fragment = xPLFragment(message)
                count = 0

            fragment[key] = value
            count += line_size

        fragments[0]['schema'] = '%s.%s' % (message.schema_class,
                                            message.schema_type)
        
        count = len(fragments)
        for idx in range(count):
            fragments[idx].number = idx + 1
            fragments[idx].count = count
            fragments[idx].message_id = message_id
            fragments[idx]['partid'] = fragments[idx].part_id
        
        return fragments

    def _combine_fragments(self, schema, parts):
        msg = xPLMessage()
        msg.schema_class, msg.schema_type = schema.split('.')
        msg.message_type = parts[0].message_type
        msg.hop = parts[0].hop
        msg.source = parts[0].source
        msg.target = parts[0].target
        
        for part in parts:
            for key, value in part.items():
                msg[key] = value
                
        return msg
    
    class _id_generator(object):
        def __init__(self):
            self.reset()
            
        def reset(self): 
            self._id = 0
            
        def next(self):
            self._id += 1
            return str(self._id)

