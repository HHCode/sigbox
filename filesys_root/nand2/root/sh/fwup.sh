sleep 5

if [ -e /mnt/OHCI/ ];then
    if [ ! -e /mnt/OHCI/`cat /var/www/version`.femet ];then
        echo "update fw..."
        tar -zxf /mnt/OHCI/*.femet -C /

        killall cpapd

        while true;
        do
            sync
            sleep 1
            /nand2/root/usr/bin/igutil.1.0 -e -s -g a 10 > /dev/null
 	    sleep 1                                        
  	    /nand2/root/usr/bin/igutil.1.0 -e -g a 10 > /dev/null
        done
    fi
else
    echo "cant find /mnt/OHCI"
fi
    
