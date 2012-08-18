#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpap.h>
#include <rs232.h>

int main( int argc , char **argv )
{

    int expectedLength;
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

    int deviceDesc;
    deviceDesc = openCPAPDevice();

    if ( sendCPAPCmd( deviceDesc , cmdBuffer , intputCount , checkedXor ) )
        exit(1);

    unsigned char responseBuffer[1024];
    int responseSize;
    responseSize = recvCPAPResponse( deviceDesc , responseBuffer , sizeof(responseBuffer) , expectedLength );

    if ( responseSize >= 0 )
    {
        print_data( responseBuffer , responseSize );
    }

    rs232_close( deviceDesc );


    return 0;
}
