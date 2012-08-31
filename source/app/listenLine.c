#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <ctype.h>


static int DEBUG=0;

#define debug_print(FMT, ARGS...) \
    do {                          \
    if (DEBUG)                     \
        fprintf(stderr, "%s:%d "FMT"", __FUNCTION__, __LINE__, ## ARGS); \
    } while (0)


#define error_print(FMT, ARGS...) \
    do {                          \
        fprintf(stderr, "%s:%d "FMT"", __FUNCTION__, __LINE__, ## ARGS); \
    } while (0)



int readline( int fd , char *buf , int size )
{
    int readSize;
    char byteData;
    int index=0;

    memset( buf , 0 , size );
    do
    {
        readSize = read( fd , &byteData , 1 );
        if ( readSize <= 0 )
        {
            error_print( "read\n" );
            return -1;
        }
        else
        {
            if ( byteData == '\r' )
            {
                debug_print("get \\r\n");
                break;
            }


            if ( byteData == '\n' )
            {
                debug_print("get \\n\n");
                break;
            }


            buf[index++] = byteData;
        }

    }while( index < size );

    if ( index >= size )
    {
        error_print( "this line too lone to store\n" );
        return -1;
    }
    else
    {
        debug_print("getline:%s\n",buf);
        return 0;
    }


}



int main( int argc , char **argv )
{
    int devFD;
    char buffer[1024];

    if (access("debug",0) == 0)
        DEBUG=1;

    if ( argc <= 1 )
        error_print( "Please input a device path\n"  );

    devFD = open( argv[1] ,  O_RDWR | O_SYNC );

    if ( devFD == 0 )
    {
        error_print( "open %s failed\n" , argv[1] );
        exit(1);
    }

    if ( readline( devFD , buffer , sizeof(buffer)) )
    {
        error_print( "readerror\n" );
    }

    close( devFD );
    printf( "%s" , buffer );

    return 0;
}
