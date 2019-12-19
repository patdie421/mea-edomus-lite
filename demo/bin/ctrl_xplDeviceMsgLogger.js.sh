#!/bin/bash

# trap '' INT
ORG="`pwd`"

SERVICE_HOMEDIR="/Users/patrice/Developpement/Logiciels/mea-edomus-lite/demo"
INTERPRETER=node
WAITTIME=5
ME=`basename $0`
PROG="xplDeviceMsgLogger.js"
PROG_OPTIONS=""

PID_FILE="$SERVICE_HOMEDIR/var/pid/$PROG.pid"
LOG_FILE="$SERVICE_HOMEDIR/var/log/$PROG.log"
ERR_FILE="$SERVICE_HOMEDIR/var/log/$PROG.err"
cd "$SERVICE_HOMEDIR"

prog_pid()
{
   sleep $WAITTIME
   _PROG="`ps aux | grep -v grep | grep -v "$ME" | grep $PROG`"
   if [ ! -z "$_PROG" ]
   then
      set $_PROG
      PID=$2
   else
      PID="false"
   fi 
   echo $PID
}


status()
{
   _PROG="`ps aux | grep -v grep | grep -v "$ME" | grep $PROG`"
   if [ -z "$_PROG" ]
   then
      echo "$PROG is not running"
      return 0
   else
      set $_PROG
      echo "$PROG is running (PID=$2)"
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
   PID=`prog_pid`
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
      echo "no $PROG process found"
   fi
}


stop()
{
   if [ -f "$PID_FILE" ]
   then
      kill `cat $PID_FILE`
      sleep $WAITTIME 
      rm "$PID_FILE"
   else
      echo "no pid file. Is $PROG realy running ?"
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
      echo "$PROG is already running"
      return 1
   fi

   $INTERPRETER bin/$PROG $PROG_OPTIONS > "$LOG_FILE" 2> "$ERR_FILE" &
   ret=$?
   if [ $ret -eq 0 ]
   then
      PID=`prog_pid`
      if [ "$PID" == "false" ]
      then
         echo "can't start $PROG. sea $LOG_FILE"
         return 1
      else
         echo -n $PID > "$PID_FILE"
      fi
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

