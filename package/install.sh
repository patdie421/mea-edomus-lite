#!/bin/bash -x

# à lancer de préférence en sudo

if [ $# -ne 1 ] && [ $# -ne 2 ]
then
   echo "usage : $0 BASEPATH [ INTERFACE ]"
   echo "suggestion : $0 /usr/local/mea-edomus"
   echo "suggestion : $0 /usr/local/mea-edomus wlan0"
   exit 1
fi

ARCH=`uname`

# recupération des parametres
INTERFACE=""
BASEPATH="$1"

if [ $# -eq 2 ]
then
   INTERFACE="$2"
fi

if [ "$INTERFACE" == "" ]
then
   if [ "$ARCH" == "Darwin" ]
   then
      INTERFACE="en0"
   else
      INTERFACE="eth0"
   fi
fi

if [ "$BASEPATH" == "/usr" ] || [ "$BASEPATH" == "/" ] || [ "$BASEPATH" == "/etc" ]
then
   echo "ERROR : can't actualy install in $BASEPATH. Choose an other directory, or install manualy !"
   exit 0
fi

if [ ! -d "$BASEPATH" ]
then
   echo "ERROR : $BASEPATH must exist. Create it !"
   exit 1
fi

if [ ! -w "$BASEPATH" ]
then
   echo "ERROR : You must have write permissions on $BASEPATH ! (use sudo)"
   exit 1
fi

NBFILEINBASEPATH=`ls -a "$BASEPATH" | wc -l`
if [ $NBFILEINBASEPATH -ne 2 ]
then
   echo "WARNING : $BASEPATH is not empty. Installation could be instable ... or other(s) application(s) could be damadged"
   while true; do
      read -p "Continue (Y/N) ? " YESNO
      case $YESNO in
        [YyOo] ) break;;
        [Nn] ) exit 1;;
        * ) echo "Please answer y or n !";;
      esac
   done
fi

if [ $ARCH != "Darwin" ]
then
   MEAUSER='mea-edomus'
   MEAGROUP='mea-edomus'

   sudo groupadd -f "$MEAUSER" > /dev/null 2>&1
   sudo useradd -r -N -g "$MEAGROUP" -M -d "$BASEPATH/var/log" -s /dev/false "$MEAUSER" > /dev/null 2>&1
   sudo usermod -a -G dialout "$MEAUSER"
fi

CURRENTPATH=`pwd`;
cd "$BASEPATH"

# Pour la création du package
# sudo strip ./bin/* # faire un strip des executables avant de faire le tar dans le makefile
# préparer les permissions avant le tar (mettre un g+s sur var/log et var/db)
# mettre root/root au repertoires/fichiers pendant le tar

if [ $ARCH == "Darwin" ]
then
   sudo tar -xpf "$CURRENTPATH"/mea-edomus.tar
else
   sudo tar --owner="$MEAUSER" --group="$MEAGROUP" -xpf "$CURRENTPATH"/mea-edomus.tar
fi

sudo ./bin/mea-edomus --autoinit --basepath="$BASEPATH" $OPTIONS

if [ $ARCH != "Darwin" ]
then
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
   sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/var/backup
   sudo chmod -R 775 "$BASEPATH"/var/backup
   sudo chown -R "$MEAUSER":"$MEAGROUP" "$BASEPATH"/etc
   sudo chmod -R 775 "$BASEPATH"/etc
fi

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
if [ -f $BASEPATH/bin/node ]
then
   sudo ./bin/mea-edomus --update --basepath="$BASEPATH" --nodejspath="$BASEPATH/bin/node"
else
   echo "No nodejs provided or found."
   echo "install one if you need the mea-edomus gui and excute $0 --basepath=\""$BASEPATH"\" --update --nodejspath=\"<PATH_TO_NODEJS>\""
fi


# pour l'instant on install pas de service Mac OS X
if [ "$ARCH" == "Darwin" ]
then
   exit 0
fi

# déclaration du service
BASEPATH4SED=`echo "$BASEPATH" | sed -e 's/\\//\\\\\\//g'`
# Pour mémoire :
# les slash doivent être transformés en \/ pour le sed suivant
# echo $BASEPATH | sed -e 's/\//\\\//g' donne le résultat attendu
# mais pour utilisation avec les `` il faut remplacer en plus les \ par des \\

# déclaration du service xPLHub
if [ "$INTERFACE" != "" ] && [ -f ./etc/init.d/xPLHub ]
then
   sudo sed -e 's/<BASEPATH>/'"$BASEPATH4SED"'/g' -e 's/<USER>/'"$MEAUSER"'/g' -e 's/<INTERFACE>/'"$INTERFACE"'/g' ./etc/init.d/xPLHub > /etc/init.d/xPLHub
   chmod +x /etc/init.d/xPLHub
   sudo update-rc.d xPLHub defaults
   sudo service xPLHub restart
fi

# déclaration du service mea-edomus
sudo sed -e 's/<BASEPATH>/'"$BASEPATH4SED"'/g' -e 's/<USER>/'"$MEAUSER"'/g' ./etc/init.d/mea-edomus > /etc/init.d/mea-edomus
chmod +x /etc/init.d/mea-edomus
sudo update-rc.d mea-edomus defaults
sudo service mea-edomus restart

cd "$CURRENTPATH"
