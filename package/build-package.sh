#!/bin/bash

if [ $# -ne 2 ]
then
   echo "usage : $0 <SOURCE_BASEPATH> <TECHNO>"
   exit 1
fi

SOURCE=$1
TECHNO=$2

if [ ! -d $SOURCE ]
then
   echo "Grrr !!!"
   exit 1
fi

cd $SOURCE

rm -r $SOURCE/package/tmp > /dev/null 2>&1

mkdir -p $SOURCE/package/tmp
mkdir -p $SOURCE/package/tmp/bin
mkdir -p $SOURCE/package/tmp/lib
mkdir -p $SOURCE/package/tmp/lib/mea-gui
mkdir -p $SOURCE/package/tmp/lib/mea-gui/maps
mkdir -p $SOURCE/package/tmp/lib/mea-plugins
mkdir -p $SOURCE/package/tmp/lib/mea-drivers
mkdir -p $SOURCE/package/tmp/lib/mea-rules
mkdir -p $SOURCE/package/tmp/etc
mkdir -p $SOURCE/package/tmp/etc/init.d
mkdir -p $SOURCE/package/tmp/var
mkdir -p $SOURCE/package/tmp/var/log
mkdir -p $SOURCE/package/tmp/var/db

chmod -R 775 $SOURCE/package/tmp/*
chmod -R g+x $SOURCE/package/tmp/*

cd $SOURCE/gui
tar cf - . | ( cd $SOURCE/package/tmp/lib/mea-gui ; tar xf - )

cd $SOURCE/plugins
tar cf - . | ( cd $SOURCE/package/tmp/lib/mea-plugins ; tar xf - )
rm $SOURCE/package/tmp/lib/mea-plugins/*.pyc

cd $SOURCE/src/interfaces
cp type_*/interface*.dylib type_*/interface*.so $SOURCE/package/tmp/lib/mea-drivers
 
cp $SOURCE/linux/init.d/* $SOURCE/package/tmp/etc/init.d

cp $SOURCE/mea-edomus.$TECHNO $SOURCE/package/tmp/bin/mea-edomus

if [ -f $SOURCE/complements/php-cgi/src/php-5.5.16/sapi/cgi/php-cgi ]
then
   cp $SOURCE/complements/php-cgi/src/php-5.5.16/sapi/cgi/php-cgi $SOURCE/package/tmp/bin
fi

if [ -f $SOURCE/complements/nodejs/src/node-v0.10.32/out/Release/node ]
then
   cp $SOURCE/complements/nodejs/src/node-v0.10.32/out/Release/node $SOURCE/package/tmp/bin
fi

if [ -f $SOURCE/complements/xplhub/src/xplhub/xplhub ]
then
   cp $SOURCE/complements/xplhub/src/xplhub/xplhub $SOURCE/package/tmp/bin
   cp $SOURCE/complements/xplhub/install/init.d/xplhub $SOURCE/package/tmp/etc/init.d
fi

if [ -f $SOURCE/complements/xPL_Hub/src/xPLLib/examples/xPL_Hub_static ]
then
   cp $SOURCE/complements/xPL_Hub/src/xPLLib/examples/xPL_Hub_static $SOURCE/package/tmp/bin
fi

if [ -f $SOURCE/complements/xPL_Hub/src/xPLLib/examples/xPL_Hub ]
then
   cp $SOURCE/complements/xPL_Hub/src/xPLLib/examples/xPL_Hub $SOURCE/package/tmp/bin
   cp $SOURCE/complements/xPL_Hub/install/init.d/xPLHub $SOURCE/package/tmp/etc/init.d
fi

if [ -f $SOURCE/commands/mea-compilr/mea-compilr ]
then
   cp $SOURCE/commands/mea-compilr/mea-compilr $SOURCE/package/tmp/bin
fi

if [ -f  $SOURCE/commands/mea-enocean/enoceanlogger ]
then
   cp $SOURCE/commands/mea-enocean/enoceanlogger $SOURCE/package/tmp/bin
fi

if [ -f  $SOURCE/commands/mea-enocean/enoceanpairing ]
then
   cp $SOURCE/commands/mea-enocean/enoceanpairing $SOURCE/package/tmp/bin
fi

if [ -f  $SOURCE/commands/mea-enocean/enoceanoutput ]
then
   cp $SOURCE/commands/mea-enocean/enoceanoutput $SOURCE/package/tmp/bin
fi

if [ -f  $SOURCE/commands/mea-xpllogger/mea-xpllogger ]
then
   cp $SOURCE/commands/mea-xpllogger/mea-xpllogger $SOURCE/package/tmp/bin
fi

strip $SOURCE/package/tmp/bin/*

cd $SOURCE/package/tmp
cd $SOURCE/package/tmp
tar cf $SOURCE/package/mea-edomus.tar *

cd $SOURCE/package
tar cjf $SOURCE/package/mea-edomus.tar.pkg.bz2 mea-edomus.tar install.sh update.sh

#rm mea-edomus.tar > /dev/null 2>&1
#rm -r $SOURCE/package/tmp > /dev/null 2>&1
