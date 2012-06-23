#include <stdio.h>
#include <stdlib.h>
#include "rs232.h"

#define DEVICE_NAME "/dev/ttyUSB0"

#ifdef DEBUG
#define printf_debug(fmt, args...) printf("%s[%d]: "fmt, __FUNCTION__, __LINE__, ##args)
#else
#define printf_debug(fmt, args...)
#endif

#define printf_error(fmt, args...) fprintf(stderr, "%s[%d]: "fmt, __FUNCTION__, __LINE__, ##args)


int main( int argc , char **argv )
{
    char inputByte;
    char outputByte;
    int expectedLength;
    int rs232_descriptor;


    if ( argc > 1 )
    {
        expectedLength= atoi( argv[1] );
    }
    else
    {
        printf_error(  "plz input expect length as argument 1\n" );
        exit(1);
    }



    rs232_descriptor = rs232_open( DEVICE_NAME , 9600 );
    if ( rs232_descriptor < 0 )
    {
        printf_error( "cant open\n"  );
        exit(1);
    }

    outputByte = getchar();
    rs232_write( rs232_descriptor , &outputByte , 1 );
    inputByte=outputByte;
    while( inputByte != EOF )
    {
        inputByte = getchar();
        if ( inputByte == '\n' )
            break;
        rs232_write( rs232_descriptor , &inputByte , 1 );
        outputByte ^= inputByte;
    }
    rs232_write( rs232_descriptor , &outputByte , 1 );
    printf_debug("\n" );


    unsigned char buffer[1024];
    int length=sizeof(buffer);
    int recv_size=0;
    int retry=100;
    int recv_return;
    do
    {
        recv_return = rs232_recv( rs232_descriptor , (char *)&buffer[recv_size] , length);

        if ( recv_return < 0 )
        {
            exit(1);
        }

        recv_size += recv_return;

        if ( retry < 50 )
            printf("remain %d bytes\n" , expectedLength-recv_size );

    }while( retry-- > 0 && recv_size < expectedLength );

    if ( retry <= 0  )
    {
        printf_debug("recv error\n" );
    }

    printf_debug( "expected value:%d,actually receive:%d\n" , expectedLength , recv_size );
    printf_debug("CPAP:");
    print_data( buffer , recv_size );
    printf_debug("\n" );


    int index;
    unsigned char xor_byte;

    xor_byte = buffer[0];
    for( index=1; index<recv_size-1 ; index++ )
    {
        xor_byte ^= buffer[index];
    }

    if ( xor_byte != buffer[recv_size-1] )
    {
        printf("error,xor should be 0x%x,but 0x%x\n" , xor_byte , buffer[recv_size-1] );
        exit(1);
    }
    else
    {
        puts("ok" );
    }


    rs232_close( rs232_descriptor );


    return 0;
}

