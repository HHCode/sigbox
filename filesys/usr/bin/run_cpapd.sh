
if [ ! -e /tmp/led ];then
    mkfifo 777 /tmp/led
fi

echo ON > /tmp/led &

/nand2/root/usr/bin/cpapd 25 &

while true;
do

    if [ "`cat /tmp/led`" == "ON" ];then
        echo led ON
        /nand2/root/usr/bin/igutil.1.0 -e -s -g a 10 > /dev/null
    else 
        echo led OFF
        /nand2/root/usr/bin/igutil.1.0 -e -g a 10 > /dev/null
    fi

done
