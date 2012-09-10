#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpap.h>
#include "rs232.h"



int openCPAPDevice( void )
{
    int usbIndex;
    int rs232_descriptor;
    for ( usbIndex=0 ; usbIndex<10 ; usbIndex++ )
    {
        char deviceName[16];
        bzero( deviceName , sizeof(deviceName) );
        sprintf( deviceName , "/dev/ttyUSB%d" , usbIndex );
        rs232_descriptor = rs232_open( deviceName , 9600 );
        if ( rs232_descriptor > 0 )
        {
            printf_debug( "open %s ok\n" , deviceName );
            return rs232_descriptor;
        }
        else
            printf_debug( "open %s error\n" , deviceName );
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
        return -2;
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
        printf_error( "Cant find CPAP device\n"  );
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

