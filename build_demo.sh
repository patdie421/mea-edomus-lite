#!/bin/bash -x

ORG=`pwd`
DEMODIR=$1
DEMOPATH=$2

MAKEFILE=Makefile.ubuntu

make -f $MAKEFILE clean-all
make -f $MAKEFILE build-all

mkdir "$DEMODIR"

cd "$ORG"
./build.sh "$DEMODIR" "$DEMOPATH"

cd "$ORG"/complements/xplhub
./build.sh "$DEMODIR" "$DEMOPATH"

cd "$ORG"/complements/mea-homebridge
./build.sh "$DEMODIR" "$DEMOPATH"

cd "$ORG"/complements/mea-xplToHomebridge
./build.sh "$DEMODIR" "$DEMOPATH"

cp scripts/*.sh "$DEMODIR" > /dev/null 2>&1
