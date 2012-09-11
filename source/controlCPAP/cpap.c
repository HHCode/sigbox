#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

#include "cpap.h"
#include "rs232.h"

#define TEST_UART_NUMBER 10
typedef struct{

    char deviceName[16];
    int descriptor;

}UartID;

typedef struct{

    UartID uart_id[TEST_UART_NUMBER];
    int is_background_free;

}cpapGlobal;

static cpapGlobal cpap_global;




int CPAP_SendCommand( int deviceDesc , char *command_code , int command_length , uint8_t *recv_buffer , int recv_buffer_length , int expected_recv_length )
{
    char checkedXor;

    checkedXor = getCheckedXor( command_code , command_length );

    command_code[command_length]=checkedXor;
    command_length++;

    if ( sendCPAPCmd( deviceDesc , command_code , command_length ) )
        return -1;

    int responseSize;
    responseSize = recvCPAPResponse( deviceDesc , recv_buffer , recv_buffer_length , command_code[1] ,  expected_recv_length );

    return responseSize;
}

int GetCPAPDescriptor( void )
{
    int uartIndex;
    for ( uartIndex=0 ; uartIndex<TEST_UART_NUMBER ; uartIndex++ )
    {
        if ( cpap_global.uart_id[uartIndex].descriptor >= 0 )
            return cpap_global.uart_id[uartIndex].descriptor;
    }

    return -1;
}

void *functionTestUART( void *param )
{
    int uart_id_index=(int)param;
    char command_code[32]={0x93,0xaa};
    UartID *uart_id = &cpap_global.uart_id[uart_id_index];
    uint8_t recv_buffer[128];
    int is_cpap_present=0;

   // pthread_detach( pthread_self() );
    if ( CPAP_SendCommand( uart_id->descriptor , command_code , 2 , recv_buffer , sizeof( recv_buffer ) , 4 ) > 0 )
    {
        if ( recv_buffer[0] == 0x93 && recv_buffer[1] == 0xaa )
        {
            printf_debug("use uart %s , uart_id[%d].descriptor=%d\n" , uart_id->deviceName , uart_id_index , uart_id->descriptor );
            is_cpap_present=1;
        }
        else
        {
            printf_debug("close uart %s doesnt echo\n" , uart_id->deviceName );
        }
    }

    if ( is_cpap_present == 0 )
    {
        rs232_close( uart_id->descriptor );
        uart_id->descriptor = -1;
    }

    return 0;
}


int openCPAPDevice( void )
{
    int uartIndex;
    int io_descriptor;
    pthread_t threadTestUART[TEST_UART_NUMBER];

    for ( uartIndex=0 ; uartIndex<TEST_UART_NUMBER ; uartIndex++ )
    {
        threadTestUART[uartIndex] = -1;
        char *deviceName = cpap_global.uart_id[uartIndex].deviceName;

        bzero( deviceName , sizeof(deviceName) );
        sprintf( deviceName , "/dev/ttyUSB%d" , uartIndex );
        io_descriptor = rs232_open( deviceName , 9600 );

        if ( io_descriptor > 0 )
        {
            printf_debug( "open %s ok\n" , deviceName );
            cpap_global.uart_id[uartIndex].descriptor=io_descriptor;
            pthread_create( &threadTestUART[uartIndex] , 0 , functionTestUART , (void *)uartIndex );
            continue;
        }
        else
        {
            cpap_global.uart_id[uartIndex].descriptor = -1;
            printf_debug( "open %s error\n" , deviceName );
        }
    }

    for ( uartIndex=0 ; uartIndex<TEST_UART_NUMBER ; uartIndex++ )
    {
        if ( threadTestUART[uartIndex] != -1 )
            pthread_join( threadTestUART[uartIndex] , 0 );
    }

    printf("use descriptor:%d\n", GetCPAPDescriptor() );
    return GetCPAPDescriptor();
}

int isErrorCode( uint8_t test_byte )
{

    if (( test_byte & 0xf0 ) == 0xe0 )
    {
        printf_error( "error code is 0x%x\n" , test_byte );
        return 1;
    }
    return 0;
}

int recvCPAPResponse( int io_descriptor , uint8_t *responseBuffer , int responseBufferLength , uint8_t cmd_byte , int expectedLength )
{
    int recv_size=0;
    int retry=10;
    int recv_return;
    uint8_t *x93_cmd=0;
    int valid_length=0;
    int index;
    uint8_t error_code=0;
    do
    {
        recv_return = rs232_recv( io_descriptor , (char *)&responseBuffer[recv_size] , responseBufferLength-recv_size );

        if ( recv_return < 0 )
        {
            perror( "read IO error" );
            break;
        }

        recv_size += recv_return;

        if ( recv_return > 0 && debug )
        {
            char message[32];
            sprintf( message , "uart(%d) >>>\n" , io_descriptor );
            printData( (char *)responseBuffer , recv_size , message );
        }

        if ( isErrorCode( responseBuffer[0] ) )
        {
            valid_length=1;
            error_code=responseBuffer[0];
            break;
        }


        if ( x93_cmd == 0 )
        {
            for( index=0 ; index<recv_size ; index++ )
            {
                if (responseBuffer[index] == 0x93 && responseBuffer[index+1] == cmd_byte )
                {
                    x93_cmd = &responseBuffer[index];
                    printf_debug("find 0x93,%x at responseBuffer[%d]\n" , cmd_byte , ( x93_cmd - responseBuffer ) );
                }
            }
        }
        else
        {
            valid_length = recv_size - ( x93_cmd - responseBuffer );
        }



        if ( retry < 1 )
            printf_debug("remain %d bytes\n" , expectedLength-valid_length );

    }while( retry-- > 0 && valid_length < expectedLength );

    if ( recv_size <  0  )
    {
        printf_debug("recv error\n" );
        return READ_UART_ERROR;
    }
    else if ( retry < 0 )
    {
        printf_debug("cant find 0x93\n"  );
        return -1;
    }
    else if ( recv_size > 0)
    {
        if ( error_code == 0 )
        {
            printf_debug( "fd:%d,expected value:%d,actually receive:%d\n" , io_descriptor ,  expectedLength , recv_size );

            int index;
            uint8_t xor_byte;

            xor_byte = x93_cmd[0];
            for( index=1; index< expectedLength-1 ; index++ )
            {
                xor_byte ^= x93_cmd[index];
            }

            if ( xor_byte != x93_cmd[expectedLength-1] )
            {
                printf("xor should be 0x%x,but 0x%x\n" , xor_byte , x93_cmd[expectedLength-1] );
                return -1;
            }

            memcpy( responseBuffer , x93_cmd , expectedLength );
            return expectedLength;
        }
    }

    return valid_length;
}

int sendCPAPCmd( int io_descriptor , char *cmd , int cmdLength )
{
    if ( io_descriptor < 0 )
    {
        printf_debug( "Cant find CPAP device\n"  );
        return -1;
    }

    if ( rs232_write( io_descriptor , cmd , cmdLength ) == 0 )
    {
        perror( "write error" );
        printf_debug( "rs232_write err\n"  );
        return -1;
    }

    if ( debug )
    {
        char message[32];
        sprintf( message , "uart(%d) <<<\n" , io_descriptor );
        printData( cmd , cmdLength ,  message );
    }

    return 0;
}

char getCheckedXor( char *cmdBuffer , int dataSize )
{
    char checkedXor;
    int bufferIndex;

    checkedXor=cmdBuffer[0];

    for( bufferIndex=1 ; bufferIndex<dataSize ; bufferIndex++ )
        checkedXor ^= cmdBuffer[bufferIndex];

    return checkedXor;
}


// if != 0, then there is data to be read on stdin
int getchTimeOut( int timeout )
{
    // timeout structure passed into select
    struct timeval tv;
    // fd_set passed into select
    fd_set fds;
    // Set up the timeout.  here we can wait for 1 second
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // Zero out the fd_set - make sure it's pristine
    FD_ZERO(&fds);
    // Set the FD that we want to read
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    // select takes the last file descriptor value + 1 in the fdset to check,
    // the fdset for reads, writes, and errors.  We are only passing in reads.
    // the last parameter is the timeout.  select will return if an FD is ready or
    // the timeout has occurred
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    // return 0 if STDIN is not ready to be read.
    return FD_ISSET(STDIN_FILENO, &fds);
}


