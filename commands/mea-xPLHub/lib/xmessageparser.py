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

import pyparsing as pp

from xaddress import xAddress

class xMessageParserError(Exception):
    pass

class xMessageParser(object):
    def __init__(self, mode='xpl'):
        self._mode = mode

        if self._mode == 'xpl':
            self.header_start = (pp.Literal('xpl-') + \
                pp.oneOf('cmnd stat trig').setResultsName('msg_type') + \
                pp.LineEnd().suppress()) \
                .setParseAction(self.handle_header_start)

            self.source_id = pp.Word(pp.alphanums).setResultsName('sv') + \
                pp.Suppress('-') + \
                pp.Word(pp.alphanums).setResultsName('sd') + \
                pp.Suppress('.') + \
                pp.Word('_' + pp.alphanums).setResultsName('si')
                
            self.target_id = pp.Or( \
                [pp.Word(pp.alphanums).setResultsName('tv') + \
                pp.Suppress('-') + \
                pp.Word(pp.alphanums).setResultsName('td') + \
                pp.Suppress('.') + \
                pp.Word('_' + pp.alphanums).setResultsName('ti'),
                pp.Literal('*').setResultsName('target')])
            
            self.separator = '='
        else:
            self.header_start = (pp.Literal('xap-header') + \
                pp.LineEnd().suppress()) \
                .setParseAction(self.handle_header_start)

            self.source_id = pp.Word(pp.alphanums).setResultsName('sv') + \
                pp.Suppress('.') + \
                pp.Word(pp.alphanums).setResultsName('sd') + \
                pp.Suppress('.') + \
                pp.Word(pp.alphanums).setResultsName('si') + \
                pp.Optional(pp.Suppress(':') + \
                pp.Word(pp.alphanums).setResultsName('subaddress'), '')
                
            self.target_id = pp.Or(
                [pp.Word(pp.alphanums).setResultsName('tv') + \
                pp.Suppress('.') + \
                pp.Word(pp.alphanums).setResultsName('td') + \
                pp.Suppress('.') + \
                pp.Word(pp.alphanums).setResultsName('ti'),
                pp.Literal('*').setResultsName('target')])
            
            self.separator = '=!'

        self.version = 'v' + pp.Suppress('=') + \
            pp.Word(pp.nums).setResultsName('version') + \
            pp.LineEnd().suppress()
            
        self.hop = 'hop' + pp.Suppress('=') + \
            pp.Word(pp.nums).setResultsName('hop') + pp.LineEnd().suppress()
            
        self.source = 'source' + \
            pp.Suppress('=') + self.source_id + pp.LineEnd().suppress()
            
        self.target = 'target' + \
            pp.Suppress('=') + self.target_id + pp.LineEnd().suppress()
            
        self.uid = 'uid' + pp.Suppress('=') + \
            pp.Word(pp.hexnums, max=8) + pp.LineEnd().suppress()
            
        self.class_ = 'class' + pp.Suppress('=') + pp.Word(pp.alphanums) + \
            '.' + pp.Word(pp.alphanums) + pp.LineEnd().suppress()

        if self._mode == 'xpl':
            self.header_body = pp.Each(
                [self.hop, self.source, self.target]
                ).setParseAction(self.handle_header_body)
        else:
            self.header_body = pp.Each(
                [self.version, self.hop, self.class_,
                self.uid, self.source,
                self.target]).setParseAction(self.handle_header_body)

        self.block_start = pp.Literal('{').suppress() + pp.LineEnd().suppress()
        self.block_end = pp.Literal('}').suppress() + pp.LineEnd().suppress()

        self.header = self.header_start + self.block_start + \
            self.header_body + self.block_end

        self.block_header = \
            (pp.Word(pp.alphanums).setResultsName('block_class') + \
             pp.Suppress('.') + \
             pp.Word(pp.alphanums).setResultsName('block_type') + \
             pp.LineEnd().suppress()).setParseAction(self.handle_block_header)

        non_separator = \
            "".join([c for c in pp.printables if c not in self.separator]) + \
            " \t"
        
        if len(self.separator) > 1:
            separator = pp.oneOf(' '.join(self.separator))
        else:
            separator = pp.Literal(self.separator)
            
        self.block_elem = (pp.Word(non_separator).setResultsName('name') +
                           separator.setResultsName('separator') +
                           pp.restOfLine.setResultsName('value') +
                           pp.LineEnd()).setParseAction(self.handle_block_elem)

        self.block_elems = pp.OneOrMore(self.block_elem)

        self.block = self.block_header + \
            self.block_start + self.block_elems + self.block_end + \
            pp.Optional(pp.LineEnd().suppress())

        self.blocks = pp.OneOrMore(self.block)

        if self._mode == 'xpl':
            self.message = self.header + pp.Optional(self.block)
        else:
            self.message = self.header + pp.Optional(self.blocks)

    def parse(self, raw, msg):
        self._msg = msg
        self.block_number = -1
        try:
            self.message.parseString(raw)
        except Exception as exc:
            raise xMessageParserError(exc.msg)

    def handle_header_start(self, toks):
        d = toks.asDict()

        if self._mode == 'xpl':
            self._msg.message_type = d['msg_type']

    def handle_header_body(self, toks):
        d = toks.asDict()
        if 'v' in d:
            self._msg.version = d['v']
            
        self._msg.hop = d['hop']

        if self._mode == 'xap' and 'class' in d:
            self._msg.schema_class, self._msg.schema_type = d['class'].split('.')

        self._msg.source = xAddress((d['sv'], d['sd'], d['si']))

        if 'target' in d and d['target'] == '*':
            self._msg.target = xAddress(('*', '*', '*'))
        else:
            self._msg.target = xAddress((d['tv'], d['td'], d['ti']))

    def handle_block_header(self, toks):
        d = toks.asDict()

        self.block_number = self.block_number + 1

        if len(self._msg.blocks) > self.block_number:
            block = self._msg.blocks[self.block_number]
        else:
            block = self._msg.add_block()

        block.schema_class = str(d['block_class']).lower()
        block.schema_type = str(d['block_type']).lower()

    def handle_block_elem(self, toks):
        d = toks.asDict()

        block = self._msg.blocks[self.block_number]
        
        name = str(d['name']).lower()
        separator = str(d['separator'])
        value = str(d['value'])
        
        if separator == '!':
            try:
                # convert string of 2 char hex digits to a bytearray
                value = bytearray([int(value[x:x+2], 16) for x in range(0, len(value), 2)])
            except:
                pass
        
        if name not in block:
            block[name] = []
            
        block[name].append(value)
