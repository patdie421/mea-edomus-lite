#!/bin/bash

usage()
{
   echo "$1 build_dir exec_dir"
   echo "   build_dir : directory where application structure will be create (ex: tmp/demo)"
   echo "   exec_dir  : path where demo application will finaly be installed (ex: /app)"
}


if [ $# -ne 2 ]
then
   usage $0
   exit 1
fi

ORG=`pwd`
REALPATH=`realpath "$0"`
BASEPATH=`dirname "$REALPATH"`
DEMODIR="$1"
DEMOPATH="$2"

MAKEFILE=Makefile.ubuntu

make -f $MAKEFILE clean-all
make -f $MAKEFILE build-all

mkdir "$DEMODIR" > /dev/null 2>&1
mkdir "$DEMODIR/scripts" > /dev/null 2>&1

cd "$BASEPATH"
./build.sh "$DEMODIR" "$DEMOPATH"

cd "$BASEPATH"/complements/xplhub
"$BASEPATH"/build.sh "$DEMODIR" "$DEMOPATH"

cd "$BASEPATH"/complements/mea-homebridge
./build.sh "$DEMODIR" "$DEMOPATH"

cd "$BASEPATH"/complements/mea-xplToHomebridge
./build.sh "$DEMODIR" "$DEMOPATH"

cd "$BASEPATH"
cp scripts/*.sh "$DEMODIR"/scripts > /dev/null 2>&1

cd "$ORG"
