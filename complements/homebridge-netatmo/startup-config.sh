NAME="ensisheim2"
TIMEZONE="Europe/Paris"

sudo raspi-config nonint do_hostname "$NAME"
sudo raspi-config nonint do_ssh 1
sudo raspi-config nonint do_boot_behaviour B1
sudo raspi-config nonint do_serial 1
sudo raspi-config nonint do_spi 1
sudo raspi-config nonint get_i2c 1
sudo raspi-config nonint do_rgpio 0
sudo raspi-config nonint do_memory_split 16
sudo timedatectl set-timezone "$TIMEZONE"

sudo apt-get -y update
sudo apt-get -y upgrade

sudo raspi-config nonint do_expand_rootfs

sudo mv /mnt/boot/startup-config.sh /mnt/boot/startup-config.sh.done

sudo reboot
