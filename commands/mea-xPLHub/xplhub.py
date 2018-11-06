#!/usr/bin/env python

#
# from http://bazaar.launchpad.net/~sffjunkie/expel/trunk/files
#

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

import os
import re
import sys
import types
import socket
import logging
import datetime
from optparse import OptionParser

from twisted.internet import reactor
from twisted.internet.error import CannotListenError
from twisted.internet.protocol import DatagramProtocol

from lib.utility import *
from lib import settings
from lib.xsource import xProxyList
from lib.xpl import xPLMessage, xPLMessageError

XPL_PORT = 3865
XPL_HBEAT_DEFAULT_INTERVAL = 10 # minutes
XPL_HBEAT_CHECK_INTERVAL = 60   # seconds


class xPLHub(object):
    """xPL hub which responds to xPL messages."""

    def __init__(self):
        self.vendor = 'sffj'
        self.device = 'hub'
        
        parser = OptionParser()
        self.add_options(parser)
        (self._options, self._args) = parser.parse_args()
        self.configure()
        
        self.source = xpl_format_source(self.vendor, self.device, self.instance)

        self._proxies = xProxyList()
        self._shutdown_started = False

    local_addresses = property(lambda self: self._addresses)
    proxies = property(lambda self: self._proxies)
    shutdown_started = property(lambda self: self._shutdown_started)
    log = property(lambda self: self._log)

    def __call__(self):
        try:
            self._log.info('xPL Hub: Starting hub %s' % self.source)
            self._log.debug('xPL Hub: Listening on port %d' % XPL_PORT)


            for address in self._listen_on:
                conn = xPLHubProtocol(hub=self)
                self._log.debug('xPL Hub: Listening on address %s' % address)
                reactor.listenUDP(XPL_PORT, conn, address)

            reactor.callLater(XPL_HBEAT_CHECK_INTERVAL, self._check_alive)
            reactor.run()

        except CannotListenError as exc:
            if exc.socketError[0] == 10048:
                self._log.warning(('xPL Hub: A hub is already listening '
                    'on port %d') % XPL_PORT)
            else:
                self._log.exception(('xPL Hub: Unhandled exception caught '
                                     'whilst starting hub.\n%s') % exc.message)

    def stop(self):
        reactor.stop()

    def shutdown(self, msg=None):
        self._shutdown_started = True

        if msg is not None:
            try:
                when = msg.blocks[0]['time']
            except:
                when = 'now'
        else:
            when = 'now'

        delay = 0.0

        log_msg = 'xPL Hub: Hub shutting down '
        if when == 'now':
            log_msg = log_msg + 'now...'
        else:
            if when.find(':') != -1:
                log_msg = log_msg + 'at %s' % when

                now = datetime.datetime.now()

                w = datetime.datetime.strptime(when, '%H:%M')
                stop_time = datetime.datetime(now.year, now.month, now.day,
                    w.hour, w.minute, 0)

                if stop_time < now:
                    stop_time = stop_time + datetime.timedelta(days=1)

                delay = (stop_time - now).seconds
            else:
                try:
                    if when.startswith('+'):
                        delay = float(when[1:])
                    else:
                        delay = float(when)

                    log_msg = log_msg + 'in %d minute(s).' % delay
                except:
                    delay = 0

                delay = delay * 60.0

        self._log.info(log_msg)

        if delay == 0.0:
            self.stop()
        else:
            reactor.callLater(delay, self.stop)

    def _check_alive(self):
        self._log.debug('xPL Hub: Checking sources alive')
        self._proxies.check_alive()
        reactor.callLater(XPL_HBEAT_CHECK_INTERVAL, self._check_alive)

    def add_options(self, parser):
        parser.add_option("-b", "--broadcast", default='255.255.255.255',
                          dest="broadcast",
                          help="Set the broadcast address.")
        
        parser.add_option("-o", "--listen-on", default=None, dest="listen_on",
                          help=("Set the interface on which to listen for "
                          "messages (PRIMARY, ANY_LOCAL, IP address or "
                          "list of IP addresses)"))
        
#        parser.add_option("-t", "--listen-to", default=None, dest="listen_to",
#                          help=("Set from where to accept messages (ANY, "
#                          "ANY_LOCAL or an IP address)"))
        
        parser.add_option("-i", "--instance",
                          dest="instance", metavar='INSTANCE', default='',
                          help="Set xpl instance to INSTANCE.")

        parser.add_option("-d", "--debug", dest="debug", action="store_true", 
                          help="Display debugging information.")
    
        parser.add_option("-f", "--file", dest="log_file", metavar='FILE',
                          default='', help="Log information to FILE.")
        
    def configure(self):
        if self._options.instance != '':
            self.instance = self._options.instance
        else:
            self.instance = fairly_unique_identifier(find_local_address())

        if self._options.debug:
            self._log_level = logging.DEBUG
        else:
            self._log_level = logging.INFO
            
        self._log_file = self._options.log_file

        self._log = logging.getLogger('')
        self._log.name = 'expel'
        self._log.setLevel(self._log_level)
        
        # Add formatter with datefmt to remove milli-seconds from stream output
        sf = logging.Formatter('%(asctime)s %(levelname)-8s %(message)s',
            datefmt="%Y-%m-%d %H:%M:%S")
        sh = logging.StreamHandler()
        sh.setFormatter(sf)
        self._log.addHandler(sh)

        if self._log_file != '':
            ff = logging.Formatter('%(asctime)s %(levelname)-8s %(message)s')
            fh = logging.FileHandler(self._log_file)
            fh.setFormatter(ff)
            self._log.addHandler(fh)

        if self._options.listen_on is None:
            self._listen_on = settings.xpl()['ListenOnAddress']
        else:
            self._listen_on = self._options.listen_on

        self._addresses = local_addresses()

        if len(self._addresses) == 0:
            self._listen_on = ['127.0.0.1']
        elif self._listen_on == '0.0.0.0' or self._listen_on == '127.0.0.1' or \
                self._listen_on in self._addresses:
            self._listen_on = [self._listen_on]
            
        # PRIMARY is a non-standard option
        elif self._listen_on == 'PRIMARY':
            self._listen_on = [self._addresses[0]]
            
        elif self._listen_on == 'ANY_LOCAL':
            self._listen_on = ['0.0.0.0']
            
        elif ',' in self._listen_on:
            ip_re = re.compile('\d{1,3}\.\d{1,3}\.d{1,3}\.d{1,3}')
            interfaces = []
            for i in self._listen_on.split(','):
                if ip_re.match(i) is not None:
                    interfaces.append(i)
            
            i1 = set(interfaces)
            i2 = set(self._addresses)
            
            self._listen_on = list(i1 & i2)
        else:
            self._log.error(('xPL Hub: Error - Invalid \'listenOn\' parameter '
                             '(%s) specified.') % str(self._listen_on))
            sys.exit()

class xPLHubProtocol(DatagramProtocol):
    """Handle xPL messages arriving on a single interface."""

    def __init__(self, hub):
        self._hub = hub
        self._local_addresses = hub.local_addresses

    def datagramReceived(self, datagram, address):
        """Called by the Twisted framework when a UDP datagram appears on
        the listening port.
        """

        try:
            if datagram[:4] == 'xpl-':
                try:
                    self._handle_datagram(datagram, address)

                except xPLMessageError as exc:
                    self._hub.log.warning(('xPL Hub: Badly formed xPL message '
                                           'received from %s:%d') % address)
                    self._hub.log.warning('xPL Hub: %s' % exc.message)
                    self._hub.log.debug('%s' % datagram)

            elif len(datagram) > 0:
                self._hub.log.warning(('xPL Hub: Unrecognized datagram '
                                       'received from %s:%d') % address)
                self._hub.log.debug('xPL Hub: %s' % datagram)
        except Exception as exc:
            self._hub.log.exception(('xPL Hub: Unhandled exception caught '
                                     'whilst processing '
                                     'message.\n%s') % exc.message)
            raise

    def _handle_datagram(self, datagram, address):
        msg = xPLMessage(raw=datagram)

        if msg.message_type == 'cmnd' and \
        msg.schema_class == 'device' and \
        msg.schema_type == 'shutdown':
            if not self._hub.shutdown_started:
                self._hub.log.info('xPL Hub: Shutdown message received.')
                reactor.callLater(1, self._hub.shutdown, msg)
            else:
                self._hub.log.info(('xPL Hub: Shutdown message received. '
                                    'Ignoring message. Shutdown already '
                                    'in progress.'))

        ip, port = address

        if 'remote-ip' in msg:
            ip = msg['remote-ip']
        else:
            ip = address[0]

        if 'port' in msg:
            port = int(msg['port'])
        else:
            port = address[1]

        if 'interval' in msg:
            interval = int(msg['interval'])
        else:
            interval = XPL_HBEAT_DEFAULT_INTERVAL
            
        source = msg.source

        local_ip = (ip in self._local_addresses) or (ip == '127.0.0.1')
        proxies = self._hub.proxies

        if local_ip:
            if (msg.schema_class == 'hbeat' or msg.schema_class == 'config'): 
                self._hub.log.debug('xPL Hub: %s-%s received from %s:%d' \
                    % (msg.message_type, msg.schema, ip, port))

                if (msg.schema_type == 'basic' or msg.schema_type == 'app'):
                    if not proxies.is_tracked(ip, port, source):
                        self._hub.log.info(('xPL Hub: Source %s on %s:%s has '
                                            'started') % (source, ip, port))
    
                    proxies.heartbeat(ip, port, source, interval)

                if msg.schema_type == 'end':
                    if proxies.is_tracked(ip, port, source):
                        self._hub.log.info(('xPL Hub: Source %s on %s:%s has '
                                            'stopped') % (source, ip, port))
                        proxies.remove(ip, port, source)

        proxies.send_datagram(datagram, self.transport)


if __name__ == '__main__':
    x = xPLHub()
    x()
