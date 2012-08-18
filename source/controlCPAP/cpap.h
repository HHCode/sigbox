#ifndef CPAP_H
#define CPAP_H
#include <common.h>

#define DEVICE_NAME "/dev/ttyUSB0"


int recvCPAPResponse( int rs232_descriptor , unsigned char *responseBuffer , int responseBufferLength , int expectedLength );
int sendCPAPCmd( int rs232_descriptor , char *cmd , int cmdLength , char checkedXor );
char getCheckedXor( char *cmdBuffer , int dataSize );
int getCmdFromStdin( char *cmdBuffer , int bufferSize );

#endif
