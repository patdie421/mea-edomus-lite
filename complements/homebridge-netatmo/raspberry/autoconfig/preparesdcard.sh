cp cmdline.txt cmdline.txt.org
sed -i 's| init=.*||' cmdline.txt
echo -n " init=\"/bin/bash -c mount /boot ; source /boot/autoconfig/rasp/raspfirstinit\"" >> cmdline.txt
