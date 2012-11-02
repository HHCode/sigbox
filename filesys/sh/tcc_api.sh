#!/bin/sh
# add sys. path
. /nand1/config/script/addsystempath.sh
sleep 1
# app version report
#. version.sh
# start microwimdows server
[ ! `pidof nano-X` ] && (./nano-X &)
sleep 1
[ ! `pidof nanowm` ] && (./nanowm &)
sleep 1
./nxcal &
# waiting for nano-X calibration done
. waitnxcal.sh
# start GUI
[ ! `pidof TCC_GUI_LAUNCHER` ] && (./TCC_GUI_LAUNCHER &)

# Dump! don't use below
# start insmod modules
# . ins-my-module.sh

. ${PWD}/renewmac.sh /nand1/etc/MACConfig

ifconfig | grep -q "eth0"
if [ "$?" = "1" ]; then
	if [ -f /usr/share/udhcpc/default.script ]; then
		echo "ETH0::DHCP Start"
		udhcpc -qb -s /usr/share/udhcpc/default.script &
	fi
fi


cat /proc/net/dev | grep "eth1" > /dev/null
if [ "$?" = "0" ]; then
	echo "ETH1::DHCP Start"
	ifconfig eth1 192.168.200.252 netmask 255.255.255.0 up && route add default gw 192.168.200.10
fi