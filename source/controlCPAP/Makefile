OBJS := cpap.o rs232.o

CFLAGS=-g  -Wall -I./
ifdef DEBUG
    CFLAGS += -DDEBUG
endif


EXE := cpapd controlCPAP

controlCPAP:sendCPAPCmd.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

cpapd:cpapd.c dac.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

.o:.c
	$(CC) $(CFLAGS) -o $@ $^

all:$(EXE)

clean:
	rm -rf *.out *.o


.PHONY : all clean controlCPAP cpapd

