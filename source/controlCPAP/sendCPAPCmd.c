#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpap.h>
#include <rs232.h>

int main( int argc , char **argv )
{

    int expectedLength;
    int rs232_descriptor;
    char cmdBuffer[128];


    if ( argc > 1 )
    {
        expectedLength= atoi( argv[1] );
    }
    else
    {
        printf_error(  "plz input expect length as argument 1\n" );
        exit(1);
    }

    char checkedXor;
    int intputCount=0;
    intputCount = getCmdFromStdin( cmdBuffer , sizeof(cmdBuffer) );

    checkedXor = getCheckedXor( cmdBuffer , intputCount );

    rs232_descriptor = rs232_open( DEVICE_NAME , 9600 );


    sendCPAPCmd( rs232_descriptor , cmdBuffer , intputCount , checkedXor );

    unsigned char responseBuffer[1024];
    int responseSize;
    responseSize = recvCPAPResponse( rs232_descriptor , responseBuffer , sizeof(responseBuffer) , expectedLength );

    print_data( responseBuffer , responseSize );

    rs232_close( rs232_descriptor );


    return 0;
}
