#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpap.h>
#include <rs232.h>
#include <tcp_client.h>
#include <signal.h>
#include <sys/resource.h>

int debug;


int InputFromStdin( char *cmdBuffer , int bufferSize )
{
    int stdin_size=0;
    char inputFromStdIn;
    int minus=5;

    do
    {
        inputFromStdIn = getchar();

        printf_debug( "get 0x%x\n" , inputFromStdIn );

        if ( inputFromStdIn == '-' )
        {
            minus--;
            if ( minus <= 0 )
                break;
        }
        else
            minus=5;

        cmdBuffer[stdin_size++]=inputFromStdIn;


        if (stdin_size >= bufferSize )
        {
            printf_debug("input too long,should be less then %d\n" , sizeof(cmdBuffer ));
            return -1;
        }
    }while( strstr( cmdBuffer , "-----") == 0 );

    return stdin_size-4;
}


void sigpipe_handler(int sig)
{

}



#define CORE_FILE_PATH "./"
int SetCoreDump( void )
{
    struct rlimit CoreLimit;

    getrlimit( RLIMIT_CORE , &CoreLimit );
    printf_debug("GetCore:hard=%d,soft=%d\n",  (int)CoreLimit.rlim_max , (int)CoreLimit.rlim_cur );

    CoreLimit.rlim_max=-1;
    CoreLimit.rlim_cur=-1;
    setrlimit( RLIMIT_CORE , &CoreLimit );
    getrlimit( RLIMIT_CORE , &CoreLimit );

    printf_debug("GetCore:hard=%d,soft=%d\n",  (int)CoreLimit.rlim_max , (int)CoreLimit.rlim_cur );


    FILE *CorePid;
    CorePid = fopen( "/proc/sys/kernel/core_uses_pid" , "w" );
    if ( CorePid == 0 )
    {
        printf_errno("cant open core_uses_pid file\n");
        return -1;
    }
    else
    {
        fprintf( CorePid , "0" );
        printf_debug("/proc/sys/kernel/core_uses_pid=0\n" );
        fclose( CorePid );
    }


    FILE *CorePath;
    CorePath = fopen( "/proc/sys/kernel/core_pattern" , "w" );
    if ( CorePath < 0 )
    {
        printf_errno("cant open proc file\n");
        return -1;
    }
    else
    {
        fprintf( CorePath , "%s/core.%%e" , CORE_FILE_PATH);
        printf("/proc/sys/kernel/core_pattern=%s/core.%%e\n",CORE_FILE_PATH );
        fclose( CorePath );
    }

    return 0;
}


int main( int argc , char **argv )
{
    //SetCoreDump();
    if ( access( "/etc/debug" , 0 ) == 0 )
        debug=1;
    signal(SIGPIPE, sigpipe_handler);
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
        printf("example:socketCPAP localhost 21 $expected_length\n");
        exit(1);
    }

    int stdin_size=0;
    memset( cmdBuffer , 0 , sizeof( cmdBuffer ) );
    stdin_size = InputFromStdin( (char *)cmdBuffer , sizeof(cmdBuffer) );

    int socket_fd=-1;
    int recv_size=-1;
    int nothing_times=3;
    unsigned char responseBuffer[512];
    SetCPAPDontReopen();


    if ( strstr( (char *)cmdBuffer , "status" ) == 0 )
    {
        uint8_t checkedXor;
        checkedXor = getCheckedXor( (uint8_t *)cmdBuffer , stdin_size );

        cmdBuffer[stdin_size] = checkedXor;
        stdin_size++;
    }

    int read_retry=20;
    do
    {
        if ( socket_fd == -1 )
            socket_fd = TCP_ConnectToServer( server , port );

        if ( socket_fd < 0 )
            exit(1);

        if ( TCP_Write( socket_fd , (char *)cmdBuffer , stdin_size ) == 0 )
        {
            do
            {
                if ( strstr( (char *)cmdBuffer , "status" ) )
                {
                    int retry=20;
                    memset( responseBuffer , 0 , sizeof(responseBuffer) );
                    do{
                        recv_size = TCP_Read( socket_fd , (char *)responseBuffer , sizeof( responseBuffer ) );
                    }while( recv_size == 0 && retry-- > 0 );

                    if ( recv_size < 0 )
                    {
                        printf_debug( "read status error\n" );
                        break;
                    }
                    else
                    {
                        printf("%s\n" , responseBuffer );
                    }

                }
                else
                {
                    recv_size = recvCPAPResponse( socket_fd , responseBuffer , sizeof( responseBuffer ) , cmdBuffer[1] , expected_length );
                    if ( recv_size <= 0 && recv_size == READ_NOTHING )
                    {
                        write( socket_fd , cmdBuffer , stdin_size );
                        printf_debug( "read nothing , send again %d\n" , nothing_times );
                        if ( nothing_times-- <= 0 )
                        {
                            nothing_times=3;
                            break;
                        }
                    }
                    else
                    {
                        if ( ( recv_size > 1 ) || (recv_size == 1 && responseBuffer[0] == 0xe5) )
                        {
                            if ( strstr( (char *)responseBuffer , "PAP" ) || strstr( (char *)responseBuffer , "FAILED" ) )
                                printf("%s\n",responseBuffer);
                            else
                            {
                                printData( (char *)responseBuffer , recv_size , "OK\n" , 1 );
                            }
                        }
                    }
                }

                if ( read_retry < 1 )
                    printf_debug( "retry socket recv but no response , reconnect again\n" );
            }
            while( recv_size <= 0 && read_retry-- > 0 );
        }

        close( socket_fd );
        socket_fd=-1;

    }while( recv_size <= 0 && read_retry-- > 0 );

    if ( read_retry <= 0 )
        printf_error("command error\n");
    exit(0);
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
