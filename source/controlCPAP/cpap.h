#ifndef CPAP_H
#define CPAP_H

#define DEVICE_NAME "/dev/ttyUSB0"


#ifdef DEBUG
#define printf_debug(fmt, args...) printf("%s[%d]: "fmt, __FILE__, __LINE__, ##args)
#else
#define printf_debug(fmt, args...)
#endif

#define printf_error(fmt, args...)\
    fprintf(stderr, "FAILED\n" ); \
    fprintf(stderr, "%s[%d]: "fmt, __FILE__, __LINE__, ##args); \





int recvCPAPResponse( int rs232_descriptor , unsigned char *responseBuffer , int responseBufferLength , int expectedLength );
int sendCPAPCmd( int rs232_descriptor , char *cmd , int cmdLength , char checkedXor );
char getCheckedXor( char *cmdBuffer , int dataSize );
int getCmdFromStdin( char *cmdBuffer , int bufferSize );

#endif
