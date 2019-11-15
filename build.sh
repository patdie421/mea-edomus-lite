#!/bin/bash -x

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

mkdir tmp
cp "$ORG"/package/mea-edomus.tar.pkg.bz2 tmp
cd tmp
tar xvf mea-edomus.tar.pkg.bz2
echo "Y" | /bin/bash install.sh "$DIR"
cd "$DIR"
cp "$ORG"/etc/*.json etc
cp "$ORG"/rules/* lib/mea-rules
bin/mea-compilr -d -i lib/mea-rules/demo.srules > lib/mea-rules/automator.rules

PYTHON=python
which python2.7 > /dev/null 2>&1
if [ $? -eq 0 ]
then
   PYTHON="python2.7"
else
   which python27 > /dev/null 2>&1
   if [ $? -eq 0 ]
   then
      PYTHON="python27"
   fi
fi

_DIR=`escape "$CMD_PATH"`
REGEX1="s/###SERVICE_HOMEDIR###/$_DIR/g"
PROG=mea-edomus
PROG_OPTIONS=""
_PROG_OPTIONS=`escape "$PROG_OPTIONS"`
REGEX2="s/###SERVICE###/$PROG/g"
REGEX3="s/###SERVICE_OPTIONS###/$_PROG_OPTIONS/g"
REGEX4="s/###INTERPRETER###//g"
cat "$ORG"/scripts/_ctrl.template | sed -e "$REGEX1" -e "$REGEX2" -e "$REGEX3" -e "$REGEX4" > "bin/ctrl_$PROG.sh"
cat "$ORG"/etc/_mea-edomus.json.template
cat "$ORG"/etc/_mea-edomus.json.template | sed -e "$REGEX1" -e "$REGEX2" -e "$REGEX3" > "etc/mea-edomus.json"
chmod +x "bin/ctrl_$PROG.sh"

REGEX1="s/###SERVICE_HOMEDIR###/$_DIR/g"
PROG=demo_device
PROG_OPTIONS=""
_PROG_OPTIONS=`escape "$PROG_OPTIONS"`
REGEX2="s/###SERVICE###/$PROG/g"
REGEX3="s/###SERVICE_OPTIONS###/$_PROG_OPTIONS/g"
REGEX4="s/###INTERPRETER###/$PYTHON/g"
cat "$ORG"/scripts/_demo_device.template | sed -e "$REGEX1" -e "$REGEX4" > "bin/$PROG"
chmod +x "bin/demo_device"

cd "$ORG"
