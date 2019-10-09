#!/bin/bash -x 

ORG=`pwd`

usage()
{
   echo $1 [install_dir]
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

ln -s ./node_modules/homebridge/bin/homebridge homebridge
mkdir config
mkdir var
mkdir var/log
mkdir var/pid

echo "$DIR" | sed -e "s/\//\\\\\//g" > /tmp/dir.$$.txt
REGEX="s/###HOMEBRIDEGEHOME###/`cat /tmp/dir.$$.txt`/g"
cat "$ORG"/scripts/_ctrl.template | sed -e "$REGEX" > ctrl.sh
rm /tmp/dir.$$.txt
cd "$ORG"
