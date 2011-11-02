# AverMedia Info

export SYSTEM := SIGNAL_BOX

include Rules.make

.PHONY : all exe main_exe base_exe camera_exe common_exe libs depend filesys install-kmodules install-dmodules install-exes install dvsdkuntar dvsdkbuild dvsdk dvsdkall dvsdkdel dvsdkclean lspuntar lspbuild lspcfg lsp lspall lspdel lspclean lspdistclean uboot ubootall ubootcfg ubootbuild ubootclean ramdisk cramfs clean

all:
	make package
	make lib
	make app
	make filesys
	make exe
	make install


package:
	make  -C$(PACKAGE_PATH) boa

filesys:
	@sh -x $(TARGET_B_FS)/build_fs.sh $(TARGET_FS) $(FILESYS_TAR) $(USER) $(FILESYS_CHANGE) $(BUILD_HOME)
	cp ./web_client/ajax/www/TI_Logo_160_64.yyuv420 $(TARGET_FS)/mnt/mmc/
#	@echo "Check default filesys and UnTar Basic flash image, Please wait!"
#	@if test -d $(TARGET_FS); then true; else sudo mkdir $(TARGET_FS) ; sudo tar jxpf $(FILESYS_TAR) -C $(TARGET_FS) 2>&1 >/dev/null ; sudo chown -R $(USER) $(TARGET_FS)/opt ; fi
#	@echo "Copy source filesys to target filesys, Please wait!"
#	@if test -d $(FILESYS_CHANGE) ; then \cd $(FILESYS_CHANGE)/ ; tar cf - --exclude=".svn" . | tar -C  $(TARGET_FS) -xpf - ; fi
	#@if test -d $(TARGET_FS)/var/run ; then \cd $(TARGET_FS) ; rm -rf var/run ; ln -s ../tmp var/run ; fi
#	cp -rf ./web_client/ajax/www/* ./sources/camera/ipnc_app/webdata/gui/

exe:
	@sources/camera/ipnc_app/interface/inc/sysinfo_version.sh

	make depend
	make libs
	make main_exe
	make camera_exe MAKE_TARGET=
	make common_exe MAKE_TARGET=

libs:
	make -fMAKEFILE.MK -C$(FRAMEWK_DIR) $(MAKE_TARGET)
	make -fMAKEFILE.MK -C$(FRAMEWK_DIR)/../application/test $(MAKE_TARGET)
	make -fMAKEFILE.MK -C$(BASE_DIR)/../application/ipnc $(MAKE_TARGET)


depend:
	make libs MAKE_TARGET=depend

main_exe:
	make -fMAKEFILE.MK -C$(FRAMEWK_DIR)/../application/test exe
ifeq ($(SYSTEM), WP_CAMERA)
	make -fMAKEFILE.MK -C$(BASE_DIR)/../application/ipnc/av_server exe
endif
ifeq ($(SYSTEM), WP_BASE)
	make -fMAKEFILE.MK -C$(BASE_DIR)/../applications exe
endif
ifeq ($(SYSTEM), F038_CAMERA)
	make -fMAKEFILE.MK -C$(BASE_DIR)/../application/ipnc/av_server exe
	make -fMAKEFILE.MK -C$(BASE_DIR)/../application/ipnc libs
endif
avs:
#	make -fMAKEFILE.MK -C$(BASE_DIR)/../application/ipnc/av_server clean
	make -fMAKEFILE.MK -C$(BASE_DIR)/../application/ipnc/av_server libs
	make -fMAKEFILE.MK -C$(BASE_DIR)/../application/ipnc/av_server exe
	cp -rf /home/eager/wp_896/sources/camera/av_capture/build/../bin//av_server.out /mnt/ipcamera/opt/ipnc/	
base_exe:
	make -C$(APP_HOME)/base $(MAKE_TARGET)

common_exe:
	make -C$(APP_HOME)/common $(MAKE_TARGET)

camera_exe:
	make -C$(APP_HOME)/camera $(MAKE_TARGET)

install-kmodules:
	mkdir -p $(TARGET_FS_KM_DIR)
	find $(KERNELDIR) -name "*.ko" -exec cp '{}' $(TARGET_FS_KM_DIR) \;
	#rm $(TARGET_FS_KM_DIR)/ltt*.ko

install-dmodules:
	mkdir -p $(TARGET_FS_DM_DIR)
	cp $(LINUXUTILS_INSTALL_DIR)/packages/ti/sdo/linuxutils/cmem/src/module/cmemk.ko $(TARGET_FS_DM_DIR)
	cp $(LINUXUTILS_INSTALL_DIR)/packages/ti/sdo/linuxutils/edma/src/module/edmak.ko $(TARGET_FS_DM_DIR)
	cp $(LINUXUTILS_INSTALL_DIR)/packages/ti/sdo/linuxutils/irq/src/module/irqk.ko $(TARGET_FS_DM_DIR)
	cp $(DVSDK_BASE_DIR)/dm365mm/module/dm365mmap.ko $(TARGET_FS_DM_DIR)

install-exes:
	make libs MAKE_TARGET=install
	cp $(EXE_BASE_DIR)/*.sh $(TARGET_FS_DIR)
	cp $(APP_HOME)/base/aver_sample/usbcam/*.sh $(TARGET_FS_DIR)
	cp $(EXE_BASE_DIR)/$(CONFIG)/*.out $(TARGET_FS_DIR)
ifeq ($(SYSTEM), WP_CAMERA)
	-rm $(TARGET_FS_DIR)/moduletest.out
	make camera_exe MAKE_TARGET=install
endif
ifeq ($(SYSTEM), WP_BASE)
	mkdir -p $(TARGET_FS_SHELL_DIR)
	make base_exe MAKE_TARGET=install
endif
ifeq ($(SYSTEM), F038_CAMERA)
	-rm $(TARGET_FS_DIR)/moduletest.out
	make camera_exe MAKE_TARGET=install
endif
	make common_exe MAKE_TARGET=install

install:
	make filesys
	-rm -rf $(TARGET_FS)/opt/*
	mkdir -p $(TARGET_FS_DIR)
	make install-exes
	make install-dmodules
	make install-kmodules

dvsdkuntar:
	@if test -d $(DVSDK_BASE_DIR); then true; else echo "UnTar DVSDK Sources, Please wait!" ; tar jxf $(DVSDK_TAR) -C $(TI_HOME) ; fi
	@if test -L $(DVSDK_BASE_DIR)/Rules.make; then true; else rm -f $(DVSDK_BASE_DIR)/Rules.make; ln -s $(MRULES_HOME)/DVSDK_Rules.make $(DVSDK_BASE_DIR)/Rules.make ; fi
	@if test $(DVSDK_CH_CNT) -ne 0; then \
		if test -d $(DVSDK_BASE_DIR); then \cd $(DVSDK_CHANGE)/ ; tar cf - --exclude=".svn" . | tar -C  $(DVSDK_BASE_DIR) -xpf - ; else echo "NO DVSDK source from TI"; false; fi; \
	fi

dvsdkbuild:
	@make dvsdkuntar
	make -C$(DVSDK_BASE_DIR) $(DVSDK_TARGET)

dvsdk:
	make dvsdkbuild DVSDK_TARGET=
#	make dvsdkbuild DVSDK_TARGET=install

dvsdkall:
	make dvsdkclean
	make dvsdk

dvsdkdel:
	-rm -rf $(DVSDK_BASE_DIR)

dvsdkclean:
	@if test -d $(DVSDK_BASE_DIR); then make dvsdkbuild DVSDK_TARGET=clean ; fi

lspuntar:
	@if test -d $(KERNELDIR); then true; else echo "UnTar Linux Sources, Please wait!" ; tar jxf $(KERNEL_TAR) -C $(TI_HOME) ; fi
	@if test $(KERNEL_CH_CNT) -ne 0; then \
	if test -d $(KERNELDIR); then \cd $(KERNEL_CHANGE)/ ; tar cf - --exclude=".svn" . | tar -C $(KERNELDIR) -xpf - ; else echo "NO Linux source from TI"; false; fi; \
	fi

lspbuild:
	@make lspuntar
	make -C$(KERNELDIR) ARCH=arm CROSS_COMPILE=$(MVTOOL_PREFIX) $(MAKE_TARGET)

lspcfg:
	make lspbuild MAKE_TARGET=$(KERNELCONFIG)
	#make lspbuild MAKE_TARGET=checksetconfig

lsp:
	make lspcfg #eager mark_for_test 
	make lspbuild MAKE_TARGET=uImage
	make lspbuild MAKE_TARGET=modules
	-mkdir -p $(TFTP_HOME)
	cp $(KERNELDIR)/arch/arm/boot/uImage $(TFTP_HOME)/$(KERNELNAME)

lspall:
	make lspclean
	make lsp

lspdel:
	-rm -rf $(KERNELDIR)

lspclean:
	@if test -d $(KERNELDIR); then make lspbuild MAKE_TARGET=clean ; rm -f $(TARGET_FS_KM_DIR)/*.ko ; rm -f $(TFTP_HOME)/$(KERNELNAME) ; fi

lspdistclean:
	@if test -d $(KERNELDIR); then make lspbuild MAKE_TARGET=distclean; rm -rf $(TARGET_FS_KM_DIR)/*.ko; rm -rf $(TFTP_HOME)/$(KERNELNAME); fi

uboot:
	make ubootcfg
	make ubootbuild MAKE_TARGET=

ubootall:
	make ubootclean
	make uboot

ubootcfg:
	make ubootbuild MAKE_TARGET=$(UBOOTCONFIG)

ubootbuild:
	make -C$(UBOOTDIR) ARCH=arm CROSS_COMPILE=$(MVTOOL_PREFIX) $(MAKE_TARGET)

ubootclean:
	make ubootbuild MAKE_TARGET=clean
	make ubootbuild MAKE_TARGET=distclean

ramdisk:
	-rm -f $(TFTP_HOME)/ramdisk_$(ROOTFS_TAIL).gz
	-umount rdmnt
	-rm -f ramdisk
	-rm -R -f rdmnt
	-mkdir rdmnt
	dd if=/dev/zero of=ramdisk bs=1k count=9334
	mke2fs -F -v -m0 ramdisk
	mount -o loop ramdisk rdmnt
	cp -av $(TARGET_FS)/* rdmnt
	umount rdmnt
	gzip -c -9 < ramdisk > $(TFTP_HOME)/ramdisk_$(ROOTFS_TAIL).gz
	rm ramdisk
	rm -R rdmnt

cramfs:
	rm -rf $(TARGET_FS)/etc/conf
	ln -s /mnt/nand/conf $(TARGET_FS)/etc/conf
	rm -f $(TFTP_HOME)/cramfsImage__$(ROOTFS_TAIL)
	sudo rm -R -f rdmnt
	mkdir rdmnt
	sudo cp -av $(TARGET_FS)/* rdmnt
	@sudo chown -R root:root rdmnt
	#@sudo find $(BUILD_HOME)/rdmnt -name ".svn" -exec rm -rf {} \;
	sudo /sbin/mkcramfs rdmnt $(TFTP_HOME)/cramfsImage_$(ROOTFS_TAIL)
	sudo rm -R -f rdmnt

firmwareup:
	@if test -f $(TFTP_HOME)/CrmafsKernel_$(ROOTFS_TAIL).tgz ; then rm -f $(TFTP_HOME)/CrmafsKernel_$(ROOTFS_TAIL).tgz ; fi
	@if test -f $(TFTP_HOME)/update_config ; then rm -f $(TFTP_HOME)/update_config ; fi
	@echo ""
	@if test -f $(TFTP_HOME)/$(KERNELNAME); then echo "******Got KernelImage: $(TFTP_HOME)/$(KERNELNAME)" ; else echo "!!!!!!No KernelImage found ($(TFTP_HOME)/$(KERNELNAME))" ; false ; fi
	@echo ""
	@if test -f $(TFTP_HOME)/cramfsImage_$(ROOTFS_TAIL); then echo "******Got RootFilesystem: $(TFTP_HOME)/cramfsImage_$(ROOTFS_TAIL)" ; else echo "!!!!!!No RootFilesystem found ($(TFTP_HOME)/cramfsImage_$(ROOTFS_TAIL))" ; exit 1 ; fi
	@echo ""
	@echo "******Create Firmware update image: $(TFTP_HOME)/CrmafsKernel_$(ROOTFS_TAIL).tgz . Please wait"
	@\cd $(TFTP_HOME) ; md5sum $(KERNELNAME) cramfsImage_$(ROOTFS_TAIL) > file.md5
	@\cd $(TFTP_HOME) ; echo "SYSTEM='$(SYSTEM)'" > update_config ; echo "ROOTFILE='cramfsImage_$(ROOTFS_TAIL)'" >> update_config ; echo "KERNEL='$(KERNELNAME)'" >> update_config ; echo "MD5SUM='file.md5'" >> update_config
	@\cd $(TFTP_HOME) ; tar -zcf cramfs_kernel_md5.tgz update_config file.md5 $(KERNELNAME) cramfsImage_$(ROOTFS_TAIL) > /dev/null 2>&1
	@\cd $(TFTP_HOME) ; rm -f update_config file.md5 
	@\cd $(TFTP_HOME) ; ./pack_image.sh cramfs_kernel_md5.tgz
	@echo ""

release:
	sudo rm -rf $(TARGET_FS)
	make 
	make cramfs
	make firmwareup


clean:
	make -C$(PACKAGE_PATH) clean


distclean: clean lspdel dvsdkdel
	-rm -rf $(TARGET_FS)
	-rm -rf $(TFTP_HOME)/$(KERNELNAME)
	-rm -rf $(TFTP_HOME)/cramfsImage_$(ROOTFS_TAIL)
	-rm -f  $(TFTP_HOME)/CrmafsKernel_$(ROOTFS_TAIL).zip
