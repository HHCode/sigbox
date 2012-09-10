OBJS := cpap.o rs232.o common.o

CFLAGS=-g  -Wall -I./
ifdef DEBUG
    CFLAGS += -DDEBUG
endif

CFLAGS += -DUNDEAD


EXE := cpapd controlCPAP

controlCPAP:sendCPAPCmd.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

cpapd:cpapd.c dac.c socket2uart.c tcp_server.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

socket2uart:tcp_server.c rs232.c socket2uart.c
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

.o:.c
	$(CC) $(CFLAGS) -o $@ $^

all:$(EXE)

clean:
	rm -rf *.out *.o
	rm socket2uart cpapd controlCPAP

.PHONY : all clean controlCPAP cpapd

