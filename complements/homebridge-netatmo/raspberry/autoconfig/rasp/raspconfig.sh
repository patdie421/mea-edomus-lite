source /boot/autoconfig/params

sudo raspi-config nonint do_hostname "$RASP_HOSTNAME"
sudo raspi-config nonint do_ssh 1
sudo raspi-config nonint do_boot_behaviour B1
sudo raspi-config nonint do_serial 1
sudo raspi-config nonint do_spi 1
sudo raspi-config nonint get_i2c 1
sudo raspi-config nonint do_rgpio 0
sudo raspi-config nonint do_memory_split 16
sudo timedatectl set-timezone "$RASP_TIMEZONE"

sudo raspi-config nonint do_expand_rootfs
