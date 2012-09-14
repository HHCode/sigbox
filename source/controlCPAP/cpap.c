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
    int uart_fd;
    int dont_reopen;

}cpapGlobal;

static cpapGlobal cpap_global;


static void tryToOpenCPAP( void )
{
    if ( cpap_global.dont_reopen == 0 )
    {
        int retry;
        for( retry=10 ; retry>0 ; retry-- )
        {
            printf_error( "try to reopen uart %d\n", retry );
            cpap_global.uart_fd = openCPAPDevice();
            if ( cpap_global.uart_fd > 0 ) break;
            sleep(1);
        }
    }
}

int CPAP_recv( int descriptor , uint8_t *cmd , int cmd_length )
{
    if ( cpap_global.uart_fd > 0 )
        descriptor = cpap_global.uart_fd;

    if ( descriptor <= 0 )
    {
        tryToOpenCPAP();
        return -1;
    }

    int recv_return;
    recv_return = rs232_recv( descriptor , (char *)cmd , cmd_length );

    if ( recv_return < 0 )
    {
        tryToOpenCPAP();

        if ( descriptor >= 0 )
            return CPAP_recv( descriptor , cmd , cmd_length );
        else
            return -1;
    }

    if ( recv_return > 0 && debug )
    {
        char message[32];
        sprintf( message , "FD(%d) >>>\n" , descriptor );
        printData( (char *)cmd , recv_return , message , 1 );
    }

    return recv_return;
}


int CPAP_send( int descriptor , char *cmd , int cmd_length )
{
    if ( cpap_global.uart_fd > 0 )
        descriptor = cpap_global.uart_fd;

    if ( descriptor <= 0 )
    {
        tryToOpenCPAP();
        return -1;
    }

    if ( rs232_write( descriptor , cmd , cmd_length ) == 0 )
    {
        perror( "write IO error" );

        tryToOpenCPAP();

        if ( descriptor >= 0 )
            return CPAP_send( descriptor , cmd , cmd_length );
        else
            return -1;
    }

    if ( debug )
    {
        char message[32];
        sprintf( message , "FD(%d) <<<\n" , descriptor );
        printData( cmd , cmd_length ,  message , 1  );
    }

    return 0;
}


static int SendCommand( int descriptor , char *command_code , int command_length , uint8_t *recv_buffer , int recv_buffer_length , int expected_recv_length )
{
    uint8_t checkedXor;

    checkedXor = getCheckedXor( (uint8_t *)command_code , command_length );

    command_code[command_length]=(char)checkedXor;
    command_length++;

    if ( rs232_write( descriptor , command_code , command_length ) == 0 )
        return -1;

    int responseSize;
    responseSize = recvCPAPResponse( descriptor , recv_buffer , recv_buffer_length , command_code[1] ,  expected_recv_length );

    return responseSize;
}


int CPAP_SendCommand( char *command_code , int command_length , uint8_t *recv_buffer , int recv_buffer_length , int expected_recv_length )
{
    uint8_t checkedXor;

    checkedXor = getCheckedXor( (uint8_t*)command_code , command_length );

    command_code[command_length]= ( char)checkedXor;
    command_length++;

    if ( CPAP_send( 0 , command_code , command_length ) )
        return -1;

    int responseSize;
    responseSize = recvCPAPResponse( cpap_global.uart_fd , recv_buffer , recv_buffer_length , command_code[1] ,  expected_recv_length );

    static int succssive_recv_zero=0;
    if ( responseSize == READ_NOTHING ) succssive_recv_zero++;
    else succssive_recv_zero=0;
    if ( succssive_recv_zero > 100 )
        tryToOpenCPAP();

    return responseSize;
}

int GetCPAPDescriptor( void )
{
    return cpap_global.uart_fd;
}

void *functionTestUART( void *param )
{
    int uart_id_index=(int)param;
    char command_code[32]={0x93,0xaa};
    UartID *uart_id = &cpap_global.uart_id[uart_id_index];
    uint8_t recv_buffer[128];
    int is_cpap_present=0;

   // pthread_detach( pthread_self() );
    if ( SendCommand( uart_id->descriptor , command_code , 2 , recv_buffer , sizeof( recv_buffer ) , 4 ) > 0 )
    {
        if ( recv_buffer[0] == 0x93 && recv_buffer[1] == 0xaa )
        {
            printf_debug("use uart %s , uart_id[%d].descriptor=%d\n" , uart_id->deviceName , uart_id_index , uart_id->descriptor );
            is_cpap_present=1;
            cpap_global.uart_fd=uart_id->descriptor;
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

    if ( cpap_global.uart_fd != -1 ) close( cpap_global.uart_fd );

    cpap_global.uart_fd=-1;

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

    int use_descriptor=GetCPAPDescriptor();

    if ( use_descriptor > 0 )
    {
        printf_debug( "use descriptor:%d\n" , use_descriptor );

    }else{
        printf_debug("cant find any cpap device\n" );
    }
    return use_descriptor;
}


void Init_CPAP( void )
{
    openCPAPDevice();
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

void SetCPAPDontReopen( void )
{
    cpap_global.dont_reopen = 1;
}

int recvCPAPResponse( int io_fd , uint8_t *responseBuffer , int responseBufferLength , uint8_t cmd_byte , int expectedLength )
{
    int recv_size=0;
    int retry=20;
    int recv_return;
    uint8_t *x93_cmd=0;
    uint8_t *e5=0;
    int valid_length=0;
    int index;
    uint8_t error_code=0;
    do
    {
        recv_return = CPAP_recv( io_fd , &responseBuffer[recv_size] , responseBufferLength-recv_size );

        recv_size += recv_return;

        if ( responseBuffer[0] != 0xe5 && isErrorCode( responseBuffer[0] ) )
        {
            valid_length=1;
            error_code=responseBuffer[0];
            break;
        }


        if ( x93_cmd == 0 && e5 == 0 )
        {
            for( index=0 ; index<recv_size ; index++ )
            {
                if (responseBuffer[index] == 0x93 && responseBuffer[index+1] == cmd_byte )
                {
                    x93_cmd = &responseBuffer[index];
                    printf_debug("find 0x93,0x%x at responseBuffer[%d]\n" , cmd_byte , ( x93_cmd - responseBuffer ) );
                }
                else if ( responseBuffer[index] == 0xe5 )
                    e5=&responseBuffer[index];

            }
        }
        else
        {
            if ( x93_cmd )
                valid_length = recv_size - ( x93_cmd - responseBuffer );

            if ( e5 )
                valid_length = 1;
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
        return READ_NOTHING;
    }
    else if ( recv_size > 0)
    {
        if ( error_code == 0 && x93_cmd )
        {
            printf_debug( "fd:%d,expected value:%d,actually receive:%d\n" , io_fd ,  expectedLength , recv_size );

            uint8_t xor_byte;

            xor_byte = (uint8_t)getCheckedXor( x93_cmd , expectedLength-1 );

            if ( xor_byte != x93_cmd[expectedLength-1] )
            {
                printf_debug("xor should be 0x%x,but 0x%x\n" , xor_byte , x93_cmd[expectedLength-1] );
                return -1;
            }

            memcpy( responseBuffer , x93_cmd , expectedLength );

            return expectedLength;
        }
        else if ( e5 )
        {
            responseBuffer[0]=0xe5;
            return 1;
        }
    }

    return valid_length;
}

char getCheckedXor( uint8_t *cmdBuffer , int dataSize )
{
    uint8_t checkedXor;
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
