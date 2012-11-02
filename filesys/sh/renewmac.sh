#!/bin/sh
STDINPUT=$1
NONE="empty"
FILEPATH=${NONE}
ADDR=

if [ -f ${STDINPUT} ]; then
	FILEPATH=${STDINPUT}
fi
if [ "${FILEPATH}" == "${NONE}" ]; then
	FILEPATH=/nand1/etc/MACConfig
fi

cat /proc/net/dev | grep "eth0" > /dev/null
if [ "$?" = "1" ]; then
insmod /lib/modules/tcc_dm9000.ko
sleep 1
fi

ifconfig | grep -q "eth0"
if [ "$?" = "0" ]; then
ifconfig eth0 down
fi

if [ ! -f ${FILEPATH} ]; then
	echo "DEBUG: No have mac config file, so use default setting. use ./setmac.sh for detail"
	#ifconfig eth0 hw ether 00:12:34:56:78:00
else
	ADDR=`cat ${FILEPATH} | head -1 | tail -1`
	ifconfig eth0 hw ether ${ADDR}
	if [ "$?" = "1" ]; then
	echo "ERROR: Cannot assign requested address by ifconfig command, so use default setting"
	ifconfig eth0 hw ether 00:12:34:56:78:00
	fi
fi
sleep 1
ifconfig eth0 up