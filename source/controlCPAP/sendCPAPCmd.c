#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpap.h>
#include <rs232.h>
#include <tcp_client.h>

int debug;


int InputFromStdin( char *cmdBuffer , int bufferSize )
{
    int stdin_size=0;
    char inputFromStdIn;


    do
    {
        inputFromStdIn = getchar();

    //    printf_debug( "get 0x%x\n" , inputFromStdIn );

        if ( inputFromStdIn == '\n' ) break;

        cmdBuffer[stdin_size++]=inputFromStdIn;


        if (stdin_size >= bufferSize )
        {
            printf_debug("input too long,should be less then %d\n" , sizeof(cmdBuffer ));
            return -1;
        }
    }while( inputFromStdIn != '\n' );

    return stdin_size;
}

int main( int argc , char **argv )
{
    if ( access( "/etc/debug" , 0 ) == 0 )
        debug=1;

    int expected_length;
    uint8_t cmdBuffer[128];

#ifdef CONTROL_CPAP_WITH_SOCKET

    char server[255];
    int port;
    if ( argc == 4 )
    {
        strncpy( server , argv[1] , sizeof( server ) );
        port = atoi( argv[2] );
        expected_length = atoi( argv[3] );
    }
    else
    {
        printf_debug("example:socketCPAP localhost 21 $expected_length\n");
        exit(1);
    }

    int stdin_size=0;
    stdin_size = InputFromStdin( (char *)cmdBuffer , sizeof(cmdBuffer) );

    int socket_fd;
    socket_fd = TCP_ConnectToServer( server , port );

    if ( TCP_Write( socket_fd , (char *)cmdBuffer , stdin_size ) == 0 )
    {
        unsigned char responseBuffer[1024];
        int recv_size;
        do
        {
            recv_size = recvCPAPResponse( socket_fd , responseBuffer , sizeof( responseBuffer ) , cmdBuffer[1] , expected_length );
            if ( recv_size == READ_NOTHING )
            {
                printf_error( "read nothing\n" );
                break;
            }
            else
            {
                printf_error( "read error\n" );
                break;
            }
        }
        while( recv_size <= 0 );

        printData( (char *)responseBuffer , recv_size , "" );
    }


#else

    if ( argc == 2 )
    {
        expected_length = atoi( argv[3] );
    }
    else
    {
        printf_debug("example:$expected_length\n");
        exit(1);
    }

    int stdin_size=0;
    stdin_size = InputFromStdin( (char *)cmdBuffer , sizeof(cmdBuffer) );

    int deviceDesc;
    deviceDesc = openCPAPDevice();

    if ( CPAP_send( deviceDesc , (char *)cmdBuffer , stdin_size ) )
        exit(1);

    unsigned char responseBuffer[1024];
    int responseSize;
    responseSize = recvCPAPResponse( deviceDesc , responseBuffer , sizeof(responseBuffer) , cmdBuffer[1] , expected_length );

    if ( responseSize >= 0 )
    {
        puts("OK");
        print_data( responseBuffer , responseSize );
    }else
        puts( "FAILED" );

    rs232_close( deviceDesc );
#endif

    return 0;
}
