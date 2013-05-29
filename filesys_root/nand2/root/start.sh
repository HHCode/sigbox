#!/bin/sh
sleep 5
if [ -e /mnt/OHCI/ ];then
    if [ ! -e /mnt/OHCI/`cat /var/www/version`.femet ];then
   	echo "update fw..."
    	tar -zxf /mnt/OHCI/*.femet -C /
        sync;sync;sync;sleep 1
        reboot
    fi
fi


insmod /lib/modules/tcc_dm9000.ko
#udhcpc -qb -s /usr/share/udhcpc/default.script &
#cat /proc/net/dev
#ifconfig
#udhcpc /usr/share/udhcpc/default.script
#ping  google.com


#/nand2/root/tool/mount.sh
/nand2/root/etc/init.d/rcS

