#!/bin/bash

ORG=`pwd`

usage()
{
   echo $1 [install_dir [cmd_path]]
}

escape()
{
   echo "$1" | sed -e "s/\//\\\\\//g" > /tmp/regex.$$.txt
   E=`cat /tmp/regex.$$.txt`
   rm /tmp/regex.$$.txt
   echo "$E"
   return 0
}


if [ ! 0 -eq $# ] && [ ! 1 -eq $# ] && [ ! 2 -eq $# ]
then
   usage $0
   exit 1
fi

if [ $# -gt 0 ]
then
   DIR="$1"
   if [ ! -d "$DIR" ]
   then
      echo "install directory must exist"
      exit 1
   else
      cd "$DIR"
      if [ 0 -ne $? ]
      then
         exit 1
      fi
   fi
fi
DIR=`pwd`

if [ 2 -eq $# ]
then
   CMD_PATH="$2"
else
   CMD_PATH="$DIR"
fi

if [ ! -f package.json ]
then
   npm init -y
fi
npm install homebridge
npm install homebridge-http-notification-server
npm install homebridge-http-lightbulb
npm install homebridge-http-outlet
npm install homebridge-http-switch
npm install homebridge-http-temperature-sensor
npm install homebridge-http-humidity-sensor
npm install homebridge-notification

mkdir bin 2> /dev/null
mkdir config 2> /dev/null
mkdir var 2> /dev/null
mkdir var/log 2> /dev/null
mkdir var/pid 2> /dev/null
ln -s ../node_modules/homebridge/bin/homebridge bin/homebridge 2> /dev/null

_DIR=`escape "$CMD_PATH"`
REGEX1="s/###SERVICE_HOMEDIR###/$_DIR/g"
PROG=homebridge
PROG_OPTIONS="-U $CMD_PATH/config"
_PROG_OPTIONS=`escape "$PROG_OPTIONS"`
REGEX2="s/###SERVICE###/$PROG/g"
REGEX3="s/###SERVICE_OPTIONS###/$_PROG_OPTIONS/g"
cat "$ORG"/scripts/_ctrl.template | sed -e "$REGEX1" -e "$REGEX2" -e "$REGEX3" > "bin/ctrl_$PROG.sh"
chmod +x "bin/ctrl_$PROG.sh"
cd "$ORG"
