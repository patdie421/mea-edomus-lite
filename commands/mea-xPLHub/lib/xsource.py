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

import logging
import datetime

__all__ = ['xProxyList']


class xProxyList(object):
    def __init__(self):
        self._proxies = {}

    def is_tracked(self, ip, port, source):
        key = '%s:%d' % (ip, port)

        if key in self._proxies:
            proxy = self._proxies[key]
            return proxy.is_tracked(source)
        else:
            return False

    def heartbeat(self, ip, port, source, interval):
        key = '%s:%d' % (ip, port)

        if key in self._proxies:
            proxy = self._proxies[key]
        else:
            proxy = xIPPortProxy(ip, port)
            self._proxies[key] = proxy

        proxy.heartbeat(source, interval)

    def send_datagram(self, datagram, transport):
        """Send a datagram to all the live proxies"""

        for proxy in self._proxies.values():
            proxy.send_datagram(datagram, transport)

    def remove(self, ip, port, source):
        key = '%s:%d' % (ip, port)

        if key in self._proxies:
            proxy = self._proxies[key]
            return proxy.remove(source)
        else:
            return False

    def check_alive(self):
        to_remove = []

        for key, proxy in self._proxies.items():
            proxy.check_alive()
                
            if proxy.count == 0:
                to_remove.append((key, proxy))

        for key, proxy in to_remove:
            logging.debug('    No source(s) on %s:%d still alive.' % (proxy.ip, proxy.port))
            del self._proxies[key]


class xIPPortProxy(object):
    def __init__(self, ip, port):
        self._ip = ip
        self._port = port
        self._proxies = {}

    def count():
        def fget(self):
            return len(self._proxies)

        return locals()

    count = property(**count())

    def ip():
        def fget(self):
            return self._ip

        def fset(self, ip):
            self._ip = ip

        return locals()

    ip = property(**ip())

    def port():
        def fget(self):
            return self._port

        def fset(self, port):
            self._port = port

        return locals()

    port = property(**port())

    def is_tracked(self, source):
        return source in self._proxies

    def heartbeat(self, source, interval):
        if source in self._proxies:
            proxy = self._proxies[source]
        else:
            proxy = xSourceProxy(source, interval)
            self._proxies[source] = proxy
            
        proxy.heartbeat(interval)

    def send_datagram(self, datagram, transport):
        """Send a datagram to all the live proxies"""
        
        transport.write(datagram, (self.ip, self.port))

    def check_alive(self):
        to_remove = []

        # Build a list of sources to remove
        for name, source in self._proxies.items():
            if not source.is_alive():
                to_remove.append(name)

        # and remove them
        l = len(to_remove)
        if l > 0:
            logging.debug('    Source(s) %s on %s:%d no longer alive and will be removed' % (', '.join(map(lambda n: str(n), to_remove)), self.ip, self.port))
            for source in to_remove:
                try:
                    del self._proxies[name]
                except:
                    pass

    def remove(self, source):
        if source in self._proxies:
            del self._proxies[source]
            return True
        else:
            return False


class xSourceProxy(object):
    def __init__(self, source, interval):
        self._source = source
        self._interval = interval
        self._heartbeat_time = datetime.datetime.now()

    def heartbeat(self, interval):
        self._interval = interval
        self._heartbeat_time = datetime.datetime.now()

    def is_alive(self):
        delta = datetime.datetime.now() - self._heartbeat_time
        alive = (delta.seconds <= ((2 * self._interval) + 1) * 60)
        logging.debug('    source=%s interval=%d, age=%d, alive=%s' % (self._source, self._interval, delta.seconds, str(alive)))
        return alive

