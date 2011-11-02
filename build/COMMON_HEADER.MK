# (c) Texas Instruments 

ifndef $(COMMON_HEADER_MK)
COMMON_HEADER_MK = 1

CC=$(MVTOOL_PREFIX)gcc
AR=$(MVTOOL_PREFIX)ar
LD=$(MVTOOL_PREFIX)gcc

LIB_BASE_DIR=$(BASE_DIR)/lib
OBJ_BASE_DIR=$(BASE_DIR)/lib/obj
EXE_BASE_DIR=$(BASE_DIR)/../bin

LIB_DIR=$(LIB_BASE_DIR)/$(CONFIG)

CC_OPTS=-c -Wall -fPIC

ifeq ($(CONFIG), debug)

CC_OPTS+=-g
OPTI_OPTS=
DEFINE=-DDEBUG

else

CC_OPTS+=
OPTI_OPTS=-O3
DEFINE=

endif

ifeq ($(SYSTEM), DVR)
DEFINE+=-DDRV_EXCLUDE_IMAGE_TUNE
endif

DEFINE+=-D$(BOARD_ID)
DEFINE+=-D$(WP_BOARD_ID)

AR_OPTS=-rc
LD_OPTS=-lpthread -lpng -fPIC -lasound

FILES=$(subst ./, , $(foreach dir,.,$(wildcard $(dir)/*.c)) ) 

vpath %.a $(LIB_DIR) 

include $(BASE_DIR)/INCLUDES.MK

INCLUDE=
INCLUDE+=$(KERNEL_INC)
INCLUDE+=$(COMMON_INC)
INCLUDE+=$(OSA_INC) 
INCLUDE+=$(CMEM_INC) 
INCLUDE+=$(WPCAM_INC)
INCLUDE+=$(IMAGE_SRC_INC)
endif # ifndef $(COMMON_HEADER_MK)

