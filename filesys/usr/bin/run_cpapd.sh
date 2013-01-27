
if [ ! -e /tmp/command ];then
    mkfifo 777 /tmp/command
fi

echo ON > /tmp/command &

/nand2/root/usr/bin/cpapd 25 &

while true;
do
    command=`cat /tmp/command`
    if [ $command == "ON" ];then
        echo led ON
        /nand2/root/usr/bin/igutil.1.0 -e -s -g a 10 > /dev/null
    elif [ $command == "OFF" ];then
        echo led OFF
        /nand2/root/usr/bin/igutil.1.0 -e -g a 10 > /dev/null
    elif [ $command == "reboot" ];then
        sleep 3
        reboot
    else
        $command
    fi
done 