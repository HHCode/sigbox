#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpap.h>
#include "rs232.h"

int recvCPAPResponse( int rs232_descriptor , unsigned char *responseBuffer , int responseBufferLength , int expectedLength )
{
    int recv_size=0;
    int retry=2;
    int recv_return;
    do
    {
        recv_return = rs232_recv( rs232_descriptor , (char *)&responseBuffer[recv_size] , responseBufferLength-recv_size );

        if ( recv_return < 0 )
        {
            perror( "read rs232 error" );
            return -1;
        }

        recv_size += recv_return;

        if ( retry < 1 )
            printf_debug("remain %d bytes\n" , expectedLength-recv_size );

    }while( retry-- > 0 && recv_size < expectedLength );

    if ( retry <= 0  )
    {
        printf_debug("recv error\n" );
    }

    printf_debug( "expected value:%d,actually receive:%d\n" , expectedLength , recv_size );

    if ( responseBuffer[0] == 0xe4 )
    {
        printf_error( "This is a invalid command" );
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
        printf("FAILED\nxor should be 0x%x,but 0x%x\n" , xor_byte , responseBuffer[recv_size-1] );
        return -1;
    }
    else
    {
        puts("OK");
    }

    return recv_size;
}

int sendCPAPCmd( int rs232_descriptor , char *cmd , int cmdLength , char checkedXor )
{
    if ( rs232_descriptor < 0 )
    {
        printf_error( "cant open ttyUSB0\n"  );
        return -1;
    }

    if ( rs232_write( rs232_descriptor , cmd , cmdLength ) == 0 )
    {
        perror( "write rs232 error" );
        return -1;
    }

    rs232_write( rs232_descriptor , &checkedXor , 1 );
    printf_debug( "\n" );

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


int getCmdFromStdin( char *cmdBuffer , int bufferSize )
{
    int intputCount=0;
    char inputFromStdIn;


    do
    {
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
            return rs232_descriptor;
        else
            printf_debug( "open %s error\n" , deviceName );
    }

    printf_error( "cant open CPAP device\n"  );
    return -1;
}
