# AverMedia Info

export SYSTEM := SIGNAL_BOX

include ./build/Rules.make

.PHONY : all exe libs filesys cramfs clean

all:
	make package
	make libs
	make filesys
	make exe
	make install


install-package:
	make -C$(PACKAGE_PATH) install-thttpd


package:
	make -C$(PACKAGE_PATH) thttpd CC=gcc

filesys:

exe:
#	make libs

libs:
#	make -fMAKEFILE.MK -C$(FRAMEWK_DIR) $(MAKE_TARGET)


depend:
#	make libs MAKE_TARGET=depend

main_exe:
#	make -fMAKEFILE.MK -C$(FRAMEWK_DIR)/../application/test exe

install:
	make -C$(PACKAGE_PATH) thttpd install 


release:
	sudo rm -rf $(TARGET_FS)
	make 
	make cramfs
	make firmwareup


clean:
	make -C$(PACKAGE_PATH) clean

