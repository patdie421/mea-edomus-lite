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

class xAddress(tuple):
    """:class:`xAddress` encapsulates a 3 string xPL or a or 3 or 4 string xAP
    address and provides a custom string representation to match
    the protocol's semantics.
    
    The default address format is xPL and can be changed by specifying a
    `mode` keyword argument which can be either 'xpl' or 'xap'
    """
    
    def __new__(cls, iter_, **kwargs):
        cls.mode = kwargs.get('mode', 'xpl')
        if cls.mode == 'xap' and len(iter_) >= 4:
            return super(xAddress, cls).__new__(cls, iter_[:4])
        else:
            return super(xAddress, cls).__new__(cls, iter_[:3])
    
    def __init__(self, *args, **kwargs):
        self.mode = kwargs.get('mode', 'xpl')
        
    def __str__(self):
        if self[0] == '*' and self[1] == '*' and self[2] == '*':
            return '*'
        elif self.mode == 'xpl':
            return '%s-%s.%s' % self[:3]
        elif self.mode == 'xap' and len(self) == 4:
            return '%s.%s.%s:%s' % (self[:4])
        elif self.mode == 'xap' and len(self) == 3:
            return '%s.%s.%s' % self[:3]
        else:
            return tuple.__str__(self[:3])
        
    def __eq__(self, other):
        ok = (str(self[0]).lower() == str(other[0]).lower()) and \
             (str(self[1]).lower() == str(other[1]).lower()) and \
             (str(self[2]).lower() == str(other[2]).lower())
             
        if len(self) == 4:
            ok = ok and (str(self[3]).lower() == str(other[3]).lower())
            
        return ok
    
    vendor = property(lambda self: self[0])
    device = property(lambda self: self[1])
    instance = property(lambda self: self[2])
    subinstance = property(lambda self: self[3] if len(self) == 4 else '')
