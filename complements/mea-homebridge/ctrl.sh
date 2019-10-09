#!/bin/bash

# trap '' INT

ORG="`pwd`"
WAITTIME=5
HOMEBRIDGEHOME="/home/ec2-user/environment/mea-edomus-lite/complements/mea-homebridge"
PID_FILE="$HOMEBRIDGEHOME/var/pid/homebridge.pid"
LOG_FILE="$HOMEBRIDGEHOME/var/log/homebridge.log"
ERR_FILE="$HOMEBRIDGEHOME/var/log/homebridge.err"
cd "$HOMEBRIDGEHOME"

homebridge_pid()
{
   sleep $WAITTIME
   HOMEBRIDGE="`ps aux | grep -v grep | grep homebridge`"
   if [ ! -z "$HOMEBRIDGE" ]
   then
      set $HOMEBRIDGE
      PID=$2
   else
      PID="false"
   fi 
   echo $PID
}


status()
{
   HOMEBRIDGE="`ps aux | grep -v grep | grep homebridge`"
   if [ -z "$HOMEBRIDGE" ]
   then
      echo "homebridge is not running"
      return 0
   else
      set $HOMEBRIDGE
      echo "homebridge is running (PID=$2)"
      return 1
   fi
}

restart()
{
   stop
   sleep $WAITTIME
   start
}

forcerestart()
{
   forcestop
   sleep $WAITTIME
   start
}


forcestop()
{
   PID=`homebridge_pid`
   if [ ! "$PID" == "false" ]
   then
      kill $PID
      sleep $WAITTIME
      status > /dev/null 2>&1
      if [ 1 -eq $? ]
      then
         kill -9 $PID > /dev/null 2>&1
      fi
      rm "$PID_FILE" > /dev/null 2>&1
   else
      echo "no homebridge process found"
   fi
}


stop()
{
   if [ -f var/pid/homebridge.pid ]
   then
      kill `cat $PID_FILE`
      sleep $WAITTIME 
      rm "$PID_FILE"
   else
      echo "no pid file. Is homebridge realy running ?"
      status
      if [ 1 -eq $? ]
      then
         echo "yes, use forcestop"
      fi
   fi
}


start()
{
   if [ -f "$PID_FILE" ]
   then
      echo "hombridge is already running"
      return 1
   fi

   ./homebridge -U config > "$LOG_FILE" 2> "$ERR_FILE" &
   ret=$?
   if [ $ret -eq 0 ]
   then
      PID=`homebridge_pid`
      echo -n $PID > "$PID_FILE"
   fi
   return 0
}

case $1 in
   start) start ;;
   stop) stop ;;
   forcestop) forcestop ;;
   restart) restart ;;
   forcerestart) forcerestart ;;
   status) status ;;
   log) tail -f "$LOG_FILE" ;;
   err) tail -f "$ERR_FILE" ;;
   pid) echo `cat $PID_FILE` ;;
esac

cd "$ORG"

