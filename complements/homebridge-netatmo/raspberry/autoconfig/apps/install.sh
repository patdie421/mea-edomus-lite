ORG=`pwd`

sudo apt-get -y update
sudo apt-get -y upgrade
sudo apt-get -y install git make
sudo apt-get -y install libavahi-compat-libdnssd-dev
sudo apt-get -y install nodejs
sudo apt-get -y install npm

sudo mkdir /apps 2> /dev/null
sudo mkdir /apps/homebridge-netatmo 2> /dev/null
sudo mkdir /apps/homebridge-netatmo/data 2> /dev/null

cd /apps/homebridge-netatmo
sudo npm install homebridge
sudo npm install homebridge-netatamo
cp "$ORG"

sudo cp /boot/autoconfig/apps/config.json /apps/homebridge-netatmo/data

sudo cp /boot/autoconfig/apps/systemdfiles/homebridge /etc/defaults
sudo cp /boot/autoconfig/apps/systemdfiles/homebridge.service /etc/systemd/system
sudo systemctl daemon-reload
sudo systemctl enable homebridge
# sudo systemctl start homebridge

echo "0 4 * * * root /sbin/reboot" >> /etc/crontab
