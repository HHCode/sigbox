#
# This make variable must be set before the demos or examples
# can be built.
#

ifndef BUILD_HOME
BUILD_HOME	:= $(shell pwd)
endif

FILESYS_PATH	:= $(BUILD_HOME)/filesys
PACKAGE_PATH	:= $(BUILD_HOME)/source/package


# TI support packages
MVTOOL_DIR	:= /opt/mv_pro_5.0/montavista/pro/devkit/arm/v5t_le
DVSDK_TAR	:= $(TI_PKG)/dvsdk_22_10_01_18.tar.bz2
KERNEL_TAR	:= $(TI_PKG)/dvlsp_22_10_00_14.tar.bz2
FILESYS_TAR	:= $(TI_PKG)/dm365_flash_image_02_10_01_18.tar.bz2

# The path after untar ti's packages
DVSDK_BASE_DIR	:= $(TI_HOME)/dvsdk_2_10_01_18
KERNELDIR	:= $(TI_HOME)/linux-2.6.18_pro500
#The path for our change in ti packages
UBOOTDIR	:= $(APP_HOME)/bsp/u-boot
KERNEL_CHANGE	:= $(APP_HOME)/bsp/dvlsp
DVSDK_CHANGE	:= $(APP_HOME)/bsp/dvsdk
FILESYS_CHANGE	:= $(APP_HOME)/common/filesys
APP_LIB_DIR=$(IPNC_INTERFACE_DIR)/lib


ifeq ($(SYSTEM), EVM)
    TARGET_FS		:= $(TARGET_B_FS)/evm
    TARGET_OPT_DIR	:= opt/evm
    TARGET_FS_DIR	:= $(TARGET_FS)/$(TARGET_OPT_DIR)
    KERNELCONFIG	:= davinci_dm365_defconfig
    UBOOTCONFIG		:= davinci_dm365_evm_config
    DM365_KERNELNAME	:= uImage_DM365_EVM
    DM365_ROOTFS_TAIL	:= EVM
endif

ifeq ($(SYSTEM), WP_BASE)
    TARGET_FS		:= $(TARGET_B_FS)/wp_base
    TARGET_OPT_DIR	:= opt/wp_b
    TARGET_FS_DIR	:= $(TARGET_FS)/$(TARGET_OPT_DIR)
    KERNELCONFIG	:= davinci_dm365_wp_base_defconfig
    UBOOTCONFIG		:= davinci_dm365_wp_config
    DM365_KERNELNAME	:= uImage_DM365_WP_BASE
    DM365_ROOTFS_TAIL	:= WP_BASE
    EXEC_DIR            := $(TARGET_FS_DIR)
    #################add for av_base################
    BOARD_ID        := BOARD_AP_WP
    #IMGS_ID        := IMGS_NONE
    IMGS_ID	        := IMGS_TVP7002
    AEWB_ID	        := AEWB_NONE
    WP_BOARD_ID     := WP100_BASE
    BASE_DIR        := $(APP_HOME)/base/AV_BASE/build
    FRAMEWK_DIR     := $(APP_HOME)/camera/av_capture/framework
    EXE_BASE_DIR    := $(BASE_DIR)/../bin
    WPBASE_DIR      := $(APP_HOME)/base
    ################################################
    TARGET_FS_KM_DIR	:= $(TARGET_FS_DIR)/kmodules
    TARGET_FS_DM_DIR	:= $(TARGET_FS_DIR)/dmodules
    TARGET_FS_SHELL_DIR	:= $(TARGET_FS_DIR)/shells
endif

ifeq ($(SYSTEM), WP_CAMERA)
    TARGET_FS		:= $(TARGET_B_FS)/wp_camera
    TARGET_OPT_DIR	:= opt/ipnc
    TARGET_FS_DIR	:= $(TARGET_FS)/$(TARGET_OPT_DIR)
    KERNELCONFIG	:= davinci_dm365_wp_camera_defconfig
    UBOOTCONFIG		:= davinci_dm365_wp_config
    DM365_KERNELNAME	:= uImage_DM365_WP_CAMERA
    DM365_ROOTFS_TAIL	:= WP_CAMERA
    ######## IPNC Config #######
    # Where to copy the resulting executables and data to (when executing 'make
    # install') in a proper file structure. This EXEC_DIR should either be visible
    # from the target, or you will have to copy this (whole) directory onto the
    # target filesystem.
    EXEC_DIR		:= $(TARGET_FS_DIR)
    # The directory that points to the IPNC software package
    IPNC_DIR		:= $(APP_HOME)/camera/ipnc_app
    BASE_DIR		:= $(APP_HOME)/camera/av_capture/build
    EXE_BASE_DIR	:= $(BASE_DIR)/../bin
    FRAMEWK_DIR		:= $(APP_HOME)/camera/av_capture/framework
    # The directory to application interface
    IPNC_INTERFACE_DIR  := $(IPNC_DIR)/interface
    # The directory to application include
    #PUBLIC_INCLUDE_DIR	:= $(IPNC_DIR)/include
    PUBLIC_INCLUDE_DIR  := $(IPNC_INTERFACE_DIR)/inc
    # The directory to application library
    LIB_DIR=$(IPNC_DIR)/interface/lib
    # The directory to root file system
    ROOT_FILE_SYS	:= $(TARGET_FS)
    BOARD_ID		:= BOARD_AP_WP
    #IMGS_ID		:= IMGS_MICRON_MT9P031_5MP
	IMGS_ID		:= IMGS_OV2715
    #LENS_ID		:= LENS_AF_MOTOR
    LENS_ID		:= LENS_NONE
    AEWB_ID         := AEWB_ENABLE
    WP_BOARD_ID     := WP100_CAMERA
#    AEWB_ID		:= AEWB_AVER
#    AEWB_ID		:= AEWB_AP
#    AEWB_LIBS		:= $(shell cat $(BASE_DIR)/../framework/alg/src/aewb_ap/AEWB_LIBS.TXT )
    ######## end IPNC Config #######
    TARGET_FS_KM_DIR       := $(TARGET_FS_DIR)/modules
    TARGET_FS_DM_DIR       := $(TARGET_FS_DIR)
endif

ifeq ($(SYSTEM), F038_CAMERA)
    TARGET_FS		:= $(TARGET_B_FS)/f038_camera
    TARGET_OPT_DIR	:= opt/ipnc
    TARGET_FS_DIR	:= $(TARGET_FS)/$(TARGET_OPT_DIR)
    KERNELCONFIG	:= davinci_dm368_f038_camera_defconfig
    UBOOTCONFIG		:= davinci_dm368_f038_camera_config
    KERNELNAME		:= uImage_DM368_F038_CAMERA
    ROOTFS_TAIL		:= DM368_F038_CAMERA
    ######## IPNC Config #######
    # Where to copy the resulting executables and data to (when executing 'make
    # install') in a proper file structure. This EXEC_DIR should either be visible
    # from the target, or you will have to copy this (whole) directory onto the
    # target filesystem.
    EXEC_DIR		:= $(TARGET_FS_DIR)
    # The directory that points to the IPNC software package
    IPNC_DIR		:= $(APP_HOME)/camera/ipnc_app
    BASE_DIR		:= $(APP_HOME)/camera/av_capture/build
    EXE_BASE_DIR	:= $(BASE_DIR)/../bin
    FRAMEWK_DIR		:= $(APP_HOME)/camera/av_capture/framework
    # The directory to application interface
    IPNC_INTERFACE_DIR  := $(IPNC_DIR)/interface
    # The directory to application include
    #PUBLIC_INCLUDE_DIR	:= $(IPNC_DIR)/include
    PUBLIC_INCLUDE_DIR  := $(IPNC_INTERFACE_DIR)/inc
    # The directory to application library
    LIB_DIR=$(IPNC_DIR)/interface/lib
    # The directory to root file system
    ROOT_FILE_SYS	:= $(TARGET_FS)
    BOARD_ID		:= BOARD_AP_WP
    #IMGS_ID		:= IMGS_MICRON_MT9P031_5MP
	IMGS_ID		:= IMGS_OV2715
    #LENS_ID		:= LENS_AF_MOTOR
    LENS_ID		:= LENS_NONE
    AEWB_ID         := AEWB_ENABLE
    WP_BOARD_ID     := WP100_CAMERA
#    AEWB_ID		:= AEWB_AVER
#    AEWB_ID		:= AEWB_AP
#    AEWB_LIBS		:= $(shell cat $(BASE_DIR)/../framework/alg/src/aewb_ap/AEWB_LIBS.TXT )
    ######## end IPNC Config #######
    TARGET_FS_KM_DIR       := $(TARGET_FS_DIR)/modules
    TARGET_FS_DM_DIR       := $(TARGET_FS_DIR)
endif


export TARGET_FS
export BUILD_HOME
export IMGS_ID
export LENS_ID
export AEWB_ID
export BOARD_ID
export AEWB_LIBS
export KERNELDIR
export MVTOOL_PREFIX
# export SYSTEM
export BASE_DIR
export IPNC_DIR
export EXEC_DIR
export WPBASE_DIR
export MVTOOL_DIR
export EXE_BASE_DIR
export TARGET_FS_DIR
export DVSDK_BASE_DIR
export DVSDK_DEMOS_DIR
export FRAMEWK_DIR
export CMEM_INSTALL_DIR
export PUBLIC_INCLUDE_DIR
export APP_LIB_DIR
export LINUXKERNEL_INSTALL_DIR
export WP_BOARD_ID
