#ifndef CPAP_H
#define CPAP_H
#include "common.h"
#include <stdint.h>
#define DEVICE_NAME "/dev/ttyUSB%d"

#define READ_UART_ERROR -2
#define READ_NOTHING    -3

int recvCPAPResponse( int io_fd  , uint8_t *responseBuffer , int responseBufferLength , uint8_t cmd_byte , int expectedLength );
int CPAP_send( int descriptor , char *cmd, int cmd_length );
char getCheckedXor( uint8_t *cmdBuffer , int dataSize );
int getCmdFromStdin( char *cmdBuffer , int bufferSize );
int CPAP_SendCommand( char *command_code , int command_length , uint8_t *recv_buffer , int recv_buffer_length , int expected_recv_length );
int openCPAPDevice( void );
int GetCPAPDescriptor( void );
int isErrorCode( uint8_t test_byte );
int CPAP_recv( int descriptor , uint8_t *cmd, int cmd_length );
void Init_CPAP( void );
void SetCPAPDontReopen( void );
#endif
