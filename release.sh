mkdir /nand2/root
mkdir /nand2/usb
mount -t vfat /dev/sda1 /nand2/usb
cd /nand2/usb
tar -xf file.tar -C ../root/
