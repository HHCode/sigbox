#include ../tcc8900.mk
#include $(TOP)config
#include $(TOP)rule.mk

INCLUDE += -I$(TOP)common
INCLUDE += -I$(KDIR)arch/arm/include

TARGET = wd_test

SOURCE_FILES += wd_test.c

all:
	$(CC) $(INCLUDE) $(CFLAGS) $(LDFLAGS) $(SOURCE_FILES) $(LIBS) -o $(TARGET)
	$(STRIP) $(TARGET)	

clean:
	rm -f wd_test

