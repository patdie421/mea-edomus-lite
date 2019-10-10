#!/bin/bash

ORG=`pwd`

usage()
{
   echo $1 [install_dir]
}

escape()
{
   echo "$1" | sed -e "s/\//\\\\\//g" > /tmp/regex.$$.txt
   E=`cat /tmp/regex.$$.txt`
   rm /tmp/regex.$$.txt
   echo "$E"
   return 0
}


if [ ! 0 -eq $# ] && [ ! 1 -eq $# ]
then
   usage $0
   exit 1
fi

if [ 1 -eq $# ]
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


mkdir config 2> /dev/null
mkdir var 2> /dev/null
mkdir var/log 2> /dev/null
mkdir var/pid 2> /dev/null
mkdir node_modules 2> /dev/null
mkdir bin 2> /dev/null

if [ ! -f package.json ]
then
   npm init -y
fi
# npm install commander
# npm install xpl-api
cp -R "$ORG"/node_modules "$DIR"

cp "$ORG"/xplDeviceMsgLogger.js "$ORG"/xplHttpBridge.js "$ORG"/xplToHomebridge.js "$DIR"/bin
cp "$ORG"/config/demo.json "$DIR"/config

_DIR=`escape "$DIR"`
REGEX1="s/###SERVICE_HOMEDIR###/$_DIR/g"

PROG=xplToHomebridge.js
PROG_OPTIONS="--cfgFile config/demo.json -p 8081"
_PROG_OPTIONS=`escape "$PROG_OPTIONS"`
REGEX2="s/###SERVICE###/$PROG/g"
REGEX3="s/###SERVICE_OPTIONS###/$_PROG_OPTIONS/g"
cat "$ORG"/scripts/_ctrl.template | sed -e "$REGEX1" -e "$REGEX2" -e "$REGEX3" > "bin/ctrl_$PROG.sh"
chmod +x "bin/ctrl_$PROG.sh"

PROG=xplHttpBridge.js
PROG_OPTIONS=""
_PROG_OPTIONS=`escape "$PROG_OPTIONS"`
REGEX2="s/###SERVICE###/$PROG/g"
REGEX3="s/###SERVICE_OPTIONS###/$_PROG_OPTIONS/g"
cat "$ORG"/scripts/_ctrl.template | sed -e "$REGEX1" -e "$REGEX2" -e "$REGEX3" > "bin/ctrl_$PROG.sh"
chmod +x "bin/ctrl_$PROG.sh"

PROG=xplDeviceMsgLogger.js
PROG_OPTIONS=""
_PROG_OPTIONS=`escape "$PROG_OPTIONS"`
REGEX2="s/###SERVICE###/$PROG/g"
REGEX3="s/###SERVICE_OPTIONS###/$_PROG_OPTIONS/g"
cat "$ORG"/scripts/_ctrl.template | sed -e "$REGEX1" -e "$REGEX2" -e "$REGEX3" > "bin/ctrl_$PROG.sh"
chmod +x "bin/ctrl_$PROG.sh"

cd "$ORG"
