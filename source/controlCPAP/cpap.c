#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpap.h>
#include "rs232.h"
#include <pthread.h>
#include <stdint.h>


#define TEST_UART_NUMBER 10
typedef struct{

    char deviceName[16];
    int descriptor;

}UartID;

typedef struct{

    UartID uart_id[TEST_UART_NUMBER];
    int inuse_uart_index;

}cpapGlobal;

static cpapGlobal cpap_global={
    inuse_uart_index:-1
};




int CPAP_SendCommand( int deviceDesc , char *command_code , int command_length , char *recv_buffer , int recv_buffer_length , int expected_recv_length )
{
    char checkedXor;

    checkedXor = getCheckedXor( command_code , command_length );

    command_code[command_length] = checkedXor;
    command_length++;

    if ( sendCPAPCmd( deviceDesc , command_code , command_length , checkedXor ) )
        return -1;

    int responseSize;
    responseSize = recvCPAPResponse( deviceDesc , recv_buffer , recv_buffer_length , expected_recv_length );

    return responseSize;
}

int GetCPAPDescriptor( void )
{
    if ( cpap_global.inuse_uart_index > 0 )
        return cpap_global.uart_id[cpap_global.inuse_uart_index].descriptor;
    else
        return 0;
}

void *functionTestUART( void *param )
{
    int uart_id_index=*(int *)param;
    char command_code[2]={0x93,0xaa};
    UartID *uart_id = &cpap_global.uart_id[uart_id_index];
    char recv_buffer[128];

    pthread_detach( pthread_self() );
    if ( CPAP_SendCommand( uart_id->descriptor , command_code , 2 , recv_buffer , sizeof( recv_buffer ) , 4 ) )
    {
        if ( recv_buffer[0] == 0x93 && recv_buffer[1] == 0xaa )
        {
            cpap_global.inuse_uart_index = uart_id_index;
            printf_debug("use uart %s , fd=%d\n" , uart_id->deviceName , uart_id->descriptor );
        }
        else
        {
            cpap_global.inuse_uart_index=-1;
            printf_debug("close uart %s doesnt echo\n" , uart_id->deviceName );
            rs232_close( uart_id->descriptor );
        }
    }

    return 0;
}


int openCPAPDevice( void )
{
    int uartIndex;
    int rs232_descriptor;
    pthread_t threadTestUART;
    for ( uartIndex=0 ; uartIndex<TEST_UART_NUMBER ; uartIndex++ )
    {
        char *deviceName = cpap_global.uart_id[uartIndex].deviceName;

        bzero( deviceName , sizeof(deviceName) );
        sprintf( deviceName , "/dev/ttyUSB%d" , uartIndex );
        rs232_descriptor = rs232_open( deviceName , 9600 );

        if ( rs232_descriptor > 0 )
        {
            printf_debug( "open %s ok\n" , deviceName );
            cpap_global.uart_id[uartIndex].descriptor=rs232_descriptor;
            pthread_create( &threadTestUART , 0 , functionTestUART , (void *)uartIndex );
            return cpap_global.uart_id[uartIndex].descriptor;
        }
        else
        {
            cpap_global.uart_id[uartIndex].descriptor = -1;
            printf_debug( "open %s error\n" , deviceName );
        }
    }

    return -1;
}



int recvCPAPResponse( int rs232_descriptor , char *responseBuffer , int responseBufferLength , int expectedLength )
{
    int recv_size=0;
    int retry=5;
    int recv_return;
    do
    {
        recv_return = rs232_recv( rs232_descriptor , (char *)&responseBuffer[recv_size] , responseBufferLength-recv_size );

        if ( recv_return < 0 )
        {
            perror( "read rs232 error" );
        }

        recv_size += recv_return;

        if ( retry < 1 )
            printf_debug("remain %d bytes\n" , expectedLength-recv_size );

    }while( retry-- > 0 && recv_size < expectedLength );

    if ( retry <  0  )
    {
        printf_debug("recv error\n" );
        return READ_UART_ERROR;
    }
    else if ( recv_size > 0)
    {
        printf_debug( "expected value:%d,actually receive:%d\n" , expectedLength , recv_size );

        if ( expectedLength > 0 && debug )
        {
            printData( responseBuffer , recv_size , "uart >>>");
        }

        if ( responseBuffer[0] == 0xe4 )
        {
            printf_error( "This is an invalid command" );
            return -1;
        }

        int index;
        unsigned char xor_byte;

        xor_byte = responseBuffer[0];
        for( index=1; index<recv_size-1 ; index++ )
        {
            xor_byte ^= responseBuffer[index];
        }

        if ( xor_byte != responseBuffer[recv_size-1] )
        {
            printf("xor should be 0x%x,but 0x%x\n" , xor_byte , responseBuffer[recv_size-1] );
            return -1;
        }
    }

    return recv_size;
}

int sendCPAPCmd( int rs232_descriptor , char *cmd , int cmdLength , char checkedXor )
{
    if ( rs232_descriptor < 0 )
    {
        printf_debug( "Cant find CPAP device\n"  );
        return -1;
    }

    if ( rs232_write( rs232_descriptor , cmd , cmdLength ) == 0 )
    {
        perror( "write rs232 error" );
        return -1;
    }

    if ( debug )
    {
        printData( cmd , cmdLength , "uart <<<\n");
    }

    rs232_write( rs232_descriptor , &checkedXor , 1 );

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

int getCmdFromStdin( char *cmdBuffer , int bufferSize )
{
    int intputCount=0;
    char inputFromStdIn;


    do
    {
        if (getchTimeOut(1)==0) break;
        inputFromStdIn = getchar();

        printf_debug( "get 0x%x\n" , inputFromStdIn );

        if ( inputFromStdIn == '\n' ) break;

        cmdBuffer[intputCount++]=inputFromStdIn;


        if (intputCount >= bufferSize )
        {
            printf("input too long,should be less then %d\n" , sizeof(cmdBuffer ));
            exit(1);
        }
    }while( inputFromStdIn != '\n' );

    return intputCount;
}



