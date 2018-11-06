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

from os import makedirs, chmod
from os.path import exists

try:
    import plistlib
except ImportError:
    raise ImportError("The OSX settings module requires 'plistlib'.")

class paths(object):
    def __init__(self):
        xpl_directory = '/Library/Application Support/xpl'
        self._items = {
            'xPLDirectory': '%s/' % xpl_directory,
            'PluginDirectory': '%s/plugins' % xpl_directory,
            'ConfigDirectory': '%s/config' % xpl_directory,
            'PidDirectory': '/tmp'
        }

    def __getitem__(self, key):
        return self._items[key]

    def get(self, key, default):
        return self._items.get(key, default)
        
    def create(self):
        for d in self._items.values():
            if d != '':
                if not exists(d):
                    makedirs(d)
                    chmod(d, 775)


class xpl():
    def __init__(self):
        config_file = '%s/xpl.plist' % paths()['xPLDirectory']
        
        if exists(config_file):
            config = plistlib.readPlist(config_file)
            
            try:
                broadcast = config['broadcast']
            except KeyError:
                broadcast = '255.255.255.255'
            
            try:
                listen_on = config['listen_on_address']
            except KeyError:
                listen_on = 'ANY_LOCAL'
            
            try:
                listen_to = config['listen_to_addresses']
            except KeyError:
                listen_to = 'ANY'
        else:
            broadcast = '255.255.255.255'
            listen_on = 'ANY_LOCAL'
            listen_to = 'ANY'
            
        self._items = {
            'BroadcastAddress': broadcast,
            'ListenOnAddress': listen_on,
            'ListenToAddresses': listen_to,
        }

    def __getitem__(self, key):
        return self._items[key]

    def get(self, key, default):
        return self._items.get(key, default)

    def create(self):
        config_file = '%s/xpl.plist' % paths()['xPLDirectory']
        
        if not exists(config_file):
            c = {
                'BroadcastAddress': '255.255.255.255',
                'ListenOnAddress': 'ANY_LOCAL',
                'ListenToAddresses': 'ANY',
            }
            
            plistlib.writePlist(c, config_file)
