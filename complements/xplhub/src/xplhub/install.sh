#!/bin/sh
# RedHat installer for xplhub

mkdir /usr/sbin/xpl

cp xplhub /usr/sbin/xpl/xplhub
cp xplhub.start /etc/init.d/xplhub
cp xplhub.amp /usr/sbin/xplhub

chmod 755 /usr/sbin/xplhub
chmod 755 /usr/sbin/xpl/xplhub
chmod 755 /etc/init.d/xplhub
chkconfig --add xplhub
service xplhub start
service xplhub status
