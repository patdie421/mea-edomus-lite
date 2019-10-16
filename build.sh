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

mkdir tmp
cp "$ORG"/package/mea-edomus.tar.pkg.bz2 tmp
cd tmp
tar xvf mea-edomus.tar.pkg.bz2
/bin/bash install.sh "$DIR"
cd "$DIR"
cp "$ORG"/etc/*.json etc
cp "$ORG"/rules/* lib/mea-rules
bin/mea-compilr -d -i lib/mea-rules/demo.srules > lib/mea-rules/automator.rules

_DIR=`escape "$DIR"`
REGEX1="s/###SERVICE_HOMEDIR###/$_DIR/g"
PROG=mea-edomus
PROG_OPTIONS=""
_PROG_OPTIONS=`escape "$PROG_OPTIONS"`
REGEX2="s/###SERVICE###/$PROG/g"
REGEX3="s/###SERVICE_OPTIONS###/$_PROG_OPTIONS/g"
cat "$ORG"/scripts/_ctrl.template | sed -e "$REGEX1" -e "$REGEX2" -e "$REGEX3" > "bin/ctrl_$PROG.sh"
cat "$ORG"/etc/_mea-edomus.json.template | sed -e "$REGEX1" -e "$REGEX2" -e "$REGEX3" > "etc/mea-edomus.json"
chmod +x "bin/ctrl_$PROG.sh"

REGEX1="s/###SERVICE_HOMEDIR###/$_DIR/g"
PROG=demo_device
PROG_OPTIONS=""
_PROG_OPTIONS=`escape "$PROG_OPTIONS"`
REGEX2="s/###SERVICE###/$PROG/g"
REGEX3="s/###SERVICE_OPTIONS###/$_PROG_OPTIONS/g"
REGEX4="s/###INTERPRETER###/python27/g"
cat "$ORG"/scripts/_demo_device.template | sed -e "$REGEX1" -e "$REGEX4" > "bin/$PROG"
chmod +x "bin/demo_device"

cd "$ORG"
