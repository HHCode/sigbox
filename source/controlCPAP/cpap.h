#ifndef CPAP_H
#define CPAP_H
#include "common.h"
#include <stdint.h>
#define DEVICE_NAME "/dev/ttyUSB%d"

#define READ_UART_ERROR -2

int recvCPAPResponse(int rs232_descriptor , uint8_t *responseBuffer , int responseBufferLength , int expectedLength );
int sendCPAPCmd( int rs232_descriptor , char *cmd , int cmdLength );
char getCheckedXor( char *cmdBuffer , int dataSize );
int getCmdFromStdin( char *cmdBuffer , int bufferSize );
int CPAP_SendCommand( int deviceDesc , char *command_code , int command_length , uint8_t *recv_buffer , int recv_buffer_length , int expected_recv_length );
int openCPAPDevice( void );
int GetCPAPDescriptor( void );
int isErrorCode( uint8_t test_byte );
#endif
