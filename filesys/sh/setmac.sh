#!/bin/sh
STDINPUT=$1
ME="$(basename "$(test -L $0 && readlink $0 || echo $0)")"
LINECNT=`echo ${STDINPUT} | sed 's/:/\n/g' | wc -l | awk '{print $1}'`
TIMELAST=
TIMENOW=
TIMESPEND=
i=1
LINE=
VALUE=
error=0
MACLOG=/nand1/etc/MACConfig
LOGFOLDER=`dirname $MACLOG`

#echo "Confirm the legitimacy of input MAC address ..."
TIMELAST=`date +"%s"`

if   [ "${STDINPUT}" = "-h" ] || [ "${STDINPUT}" = "--help" ] ; then
	echo "       Usage Sample: ${ME} MACADDR"
elif [ ${LINECNT} -ne 6 ] ; then
	echo "ERROR: input mac address length wrong: ${STDINPUT}"
	echo "       Usage Sample: ${ME} 00:11:cc:bb:77:ff"

else
while [ $i -le ${LINECNT} ] ; do
	LINE=`echo ${STDINPUT} | sed 's/:/\n/g' | head -$i | tail -1`
	VALUE=`printf "%d" 0x${LINE}`
        if [ ${VALUE} -lt 0 ] && [ ${VALUE} -gt 255 ] ; then
		echo "${LINE} is less than 0 or bigget than 255"
		error=`expr $error + 1`

	
	fi
	i=`expr $i + 1`
done
fi
TIMENOW=`date +"%s"`
TIMESPEND=`echo "${TIMENOW} ${TIMELAST} sub p" | dc`
echo "Finish ... use ${TIMESPEND} sec."

if [ ! -d ${LOGFOLDER} ]; then
	mkdir -p ${LOGFOLDER}
fi

if [ ${error} -eq 0 ]; then
	if [ -f ${MACLOG} ]; then
		rm ${MACLOG}
	fi
	touch ${MACLOG}
	echo "save MAC setting in ${MACLOG}"
	echo "${STDINPUT}" >> ${MACLOG}
else
	echo "not update MAC setting"
fi

#. ${PWD}/renewmac.sh ${MACLOG}