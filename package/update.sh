#!/bin/bash

# à lancer de préférence en sudo

if [ $# -ne 1 ]
then
   echo "usage : $0 BASEPATH"
   exit 1
fi

ARCH=`uname`

# recupération des parametres
BASEPATH="$1"

if [ "$BASEPATH" == "/usr" ] || [ "$BASEPATH" == "/" ] || [ "$BASEPATH" == "/etc" ]
then
   echo "ERROR : can't actualy install in $BASEPATH. Choose an other directory, or install manualy !"
   exit 0
fi

if [ ! -d "$BASEPATH" ]
then
   echo "ERROR : $BASEPATH must exist."
   exit 1
fi

if [ ! -w "$BASEPATH" ]
then
   echo "ERROR : You must have write permissions on $BASEPATH ! (use sudo)"
   exit 1
fi

MEAUSER='mea-edomus'
MEAGROUP='mea-edomus'

CURRENTPATH=`pwd`;
cd "$BASEPATH"

sudo service mea-edomus stop

TM=`date +"%Y%m%d-%H%M%S"`
tar cvzf $CURRENTPATH/mea-edomus.$TM.tgz .

sudo tar --owner="$MEAUSER" --group="$MEAGROUP" -xpf "$CURRENTPATH"/mea-edomus.tar

sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/lib/mea-gui
sudo chmod -R 775 "$BASEPATH"/lib/mea-gui
sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/lib/mea-gui/maps
sudo chmod -R 775 "$BASEPATH"/lib/mea-plugins
sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/lib/mea-plugins
sudo chmod -R 775 "$BASEPATH"/lib/mea-rules
sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/lib/mea-rules
sudo chmod -R 775 "$BASEPATH"/lib/mea-plugins
sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/var/log
sudo chmod -R 775 "$BASEPATH"/var/log
sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/var/db
sudo chmod -R 775 "$BASEPATH"/var/db
sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/var/sessions
sudo chmod -R 775 "$BASEPATH"/var/sessions
sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/etc
sudo chmod -R 775 "$BASEPATH"/etc


# recherche un PHP-CGI dans le PATH si non fourni
if [ ! -f ./bin/php-cgi ]
then
   PHPCGI=`which php-cgi`
   PHPCGI=`dirname $PHPCIG`
   if [ ! -z $PHPCGI ]
   then
      sudo ./bin/mea-edomus --update --basepath="$BASEPATH" --phpcgipath="$PHPCGI"
   else
      echo "No php-cgi provided or found."
      echo "install one if you need the mea-edomus gui and excute $0 --basepath=\""$BASEPATH"\" --update --phpcgipath=\"<PATH_TO_CGI_BIN>\""
   fi
fi

# un node.js est fourni dans le package
if [ -f ./bin/node ]
then
   sudo ./bin/mea-edomus --update --basepath="$BASEPATH" --nodejspath="$BASEPATH/bin/node"
else
   echo "No nodejs provided or found."
   echo "install one if you need the mea-edomus gui and excute $0 --basepath=\""$BASEPATH"\" --update --nodejspath=\"<PATH_TO_NODEJS>\""
fi

sudo service mea-edomus start
