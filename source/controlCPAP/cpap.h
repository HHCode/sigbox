#ifndef CPAP_H
#define CPAP_H
#include <common.h>

#define DEVICE_NAME "/dev/ttyUSB%d"


int recvCPAPResponse(int rs232_descriptor , char *responseBuffer , int responseBufferLength , int expectedLength );
int sendCPAPCmd( int rs232_descriptor , char *cmd , int cmdLength , char checkedXor );
char getCheckedXor( char *cmdBuffer , int dataSize );
int getCmdFromStdin( char *cmdBuffer , int bufferSize );
int openCPAPDevice( void );
#endif
