include ../../build/Rules.make

.PHONY: boa clean install thttpd install-thttpd

all:	boa thttpd

boa:
	make -Cboa $(MAKE_TARGET)


install-thttpd:
	cp ./thttpd/thttpd $(FILESYS_PATH)/usr/bin
	cp -a ./thttpd/cgi-bin/ $(FILESYS_PATH)/var/www/
	cp ./thttpd/scripts/thttpd.sh $(FILESYS_PATH)/etc/init.d/thttpd
	cp ./thttpd/scripts/thttpd_wrapper $(FILESYS_PATH)/etc/thttpd.d/
	cp ./thttpd/contrib/redhat-rpm/thttpd.conf $(FILESYS_PATH)/etc/thttpd.d/


thttpd:
	make -Cthttpd $(MAKE_TARGET)

clean:
#	make -Cboa $(MAKE_TARGET)=clean
	make thttpd MAKE_TARGET=clean


install:
#	cp ./boa/src/boa $(FILESYS_PATH)
	make thttpd-install
