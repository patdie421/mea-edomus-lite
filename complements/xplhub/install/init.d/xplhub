#!/bin/sh

### BEGIN INIT INFO
# Provides:        xplhub
# Required-Start:  $all
# Required-Stop:   $network $syslog
# Default-Start:   2 3 4 5
# Default-Stop:    0 1 6
# Short-Description: Start xplhub daemon
### END INIT INFO

PATH=/sbin:/bin:/usr/sbin:/usr/bin

. /lib/lsb/init-functions

MEAEDOMUS_BASEPATH="<BASEPATH>"

DAEMON="$MEAEDOMUS_BASEPATH/bin/xplhub"
#XPLHUB_OPT="wlan0"
XPLHUB_OPT="<INTERFACE>"

PIDFILE=/var/run/xplhub.pid
test -x $DAEMON || exit 5

#if [ -r /etc/default/xplhub ]; then
#	. /etc/default/xplhub
#fi

RUNASUSER=<USER>
UGID=$(getent passwd $RUNASUSER | cut -f 3,4 -d:) || true

case $1 in
	start)
		log_daemon_msg "Starting xplhub" "xplhub"
		if [ -z "$UGID" ]; then
			log_failure_msg "user \"$RUNASUSER\" does not exist"
			exit 1
		fi
  		start-stop-daemon --make-pidfile --background --chuid $RUNASUSER --start --pidfile $PIDFILE --startas $DAEMON -- $XPLHUB_OPT
		status=$?
		log_end_msg $status
  		;;
	stop)
		log_daemon_msg "Stopping xplhub" "xplhub"
  		start-stop-daemon --stop --signal 3 --quiet --pidfile $PIDFILE
		log_end_msg $?
		rm -f $PIDFILE
  		;;
	restart|force-reload)
		$0 stop && sleep 2 && $0 start
  		;;
	try-restart)
		if $0 status >/dev/null; then
			$0 restart
		else
			exit 0
		fi
		;;
	status)
		status_of_proc $DAEMON "xplhub"
		;;
	*)
		echo "Usage: $0 {start|stop|restart|try-restart|force-reload|status}"
		exit 2
		;;
esac
