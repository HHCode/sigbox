#include "tcp_server.h"
#include "rs232.h"
#include <stdlib.h>
#include "stdio.h"
#include "string.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include "socket2uart.h"
#include "common.h"
#include "ctype.h"
#include "cpap.h"
#define BUF_SIZE 1024
#define SERVPORT 9527


#define PORT_OF_GETSTATUS 26


void *functionGetStatus( void *param )
{
    Socket2Uart *socket_to_uart=( Socket2Uart *)param;
    int listen_fd=-1;
    int connect_fd;
    char buffer[1024];
    while(1)
    {
        connect_fd = start_tcp_server( &listen_fd , PORT_OF_GETSTATUS );

        if ( connect_fd < 0 )
        {
            printf_error( "GetStatus:the listen socket error\n" );
            continue;
        }

        int read_size;
        read_size = read_socket( connect_fd , buffer );

        if ( read_size < 0 )
        {
            printf_error( "GetStatus:read socket error\n" );
            continue;
        }
        else if ( read_size == 0 )
        {
            printf_debug( "GetStatus:read socket[%d] time out\n" , connect_fd );
            continue;
        }

        if ( debug )
        {
            printf_debug( "GetStatus:socket[%d] >>>\n" , connect_fd );
            if ( read_size > 0 )
                printData( buffer , read_size , "" , 1  );
        }
        char status_string[128];

        if ( strstr( buffer , "status" ) )
        {
            socket2uart_GetStatusString( socket_to_uart , status_string , sizeof( status_string ));

            if ( debug )
            {
                printf_debug( "socket[%d] <<<\n" , connect_fd );
                printData( status_string , strlen( status_string ) , "" , 0  );
            }
            char output_status[256];
            if ( strlen( status_string ) > 0 )
            {
                snprintf( output_status , sizeof(output_status) , "STATUS\n%s" , status_string );
            }
            else
            {
                snprintf( output_status , sizeof(output_status) , "FAILED\nPAP is not connected\n" );
            }
            write( connect_fd , output_status , strlen( output_status ));
        }

        close( connect_fd );
    }



    return 0;
}



void Init_GetStatus( Socket2Uart *socket_to_uart )
{
    pthread_t threadGetStatus;
    pthread_create( &threadGetStatus , 0 , functionGetStatus , socket_to_uart );
}


void *functionSocket2Uart( void *param )
{
    Socket2Uart *socket_to_uart=(Socket2Uart *)param;

    socket2uart( socket_to_uart );
    return 0;
}


void Init_socket2uart( Socket2Uart *socket_to_uart )
{
    pthread_create( &socket_to_uart->threadSocket2Uart , 0 , functionSocket2Uart , socket_to_uart );
    Init_GetStatus( socket_to_uart );
}

int socket2uart_IsConnect( Socket2Uart *socket_to_uart )
{
    int connect;
    pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );
    if ( socket_to_uart->connect_fd != -1 )
        connect=1;
    else
        connect=0;
    pthread_mutex_unlock( &socket_to_uart->mutexSocket2Uart );

    return connect;
}



void socket2uart_CloseClient(  Socket2Uart *socket_to_uart )
{
    usleep(10000);
    printf_debug( "close socket[%d]\n" , socket_to_uart->connect_fd );
    if ( socket_to_uart->connect_fd >= 0 )
        close ( socket_to_uart->connect_fd );
    socket_to_uart->connect_fd = -1;
}

void socket2uart_closeForced( Socket2Uart *socket_to_uart )
{
    pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );

    char *message = "FAILED\nforced disconnect client\n";
    printf_debug( "%s" , message );

    write( socket_to_uart->connect_fd , message , strlen(message));
    socket2uart_CloseClient( socket_to_uart );
    pthread_mutex_unlock( &socket_to_uart->mutexSocket2Uart );
}

void socket2uart_setExecutePermit( Socket2Uart *socket_to_uart , int execution_permit )
{
    pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );
    socket_to_uart->execution_permit=execution_permit;
    pthread_mutex_unlock( &socket_to_uart->mutexSocket2Uart );
}

int socket2uart_getExecutePermit( Socket2Uart *socket_to_uart )
{
    int permit;
    pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );
    permit = socket_to_uart->execution_permit;
    pthread_mutex_unlock( &socket_to_uart->mutexSocket2Uart );

    return permit;
}


int socket2uart_RefreshConnectID( Socket2Uart *socket_to_uart , int *connect_serial_number )
{
    int reconnected=0;
    pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );
    if ( *connect_serial_number != socket_to_uart->connect_serial_number )
    {
        *connect_serial_number=socket_to_uart->connect_serial_number;
        reconnected=1;
        printf_debug("detect reconnected\n");
    }
    pthread_mutex_unlock( &socket_to_uart->mutexSocket2Uart );

    return reconnected;
}


void socket2uart_SetReconnected( Socket2Uart *socket_to_uart )
{
    pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );
    socket_to_uart->connect_serial_number++;
    pthread_mutex_unlock( &socket_to_uart->mutexSocket2Uart );
}



void socket2uart_SetStatusString( Socket2Uart *socket_to_uart , char *status_string )
{
    pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );
    strncpy( socket_to_uart->status_string , status_string , sizeof(socket_to_uart->status_string) );
    pthread_mutex_unlock( &socket_to_uart->mutexSocket2Uart );
}


void socket2uart_GetStatusString( Socket2Uart *socket_to_uart , char *status_string , int status_string_len )
{
    pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );
    snprintf( status_string , status_string_len , "%s,%s" , GetConnectStatus() , socket_to_uart->status_string );
    pthread_mutex_unlock( &socket_to_uart->mutexSocket2Uart );
}



static int DisconnectIfNoPermit( Socket2Uart *socket_to_uart )
{

    int retry=3;
    //Wait permit from cpapd. about 0.5s will be timeout
    while( socket2uart_getExecutePermit( socket_to_uart ) == 0 && retry-- > 0 )
        usleep( 50000 );
    if ( retry <=0 )
    {
        char *message = "FAILED\nclose client due to no permit\n";
        printf_debug( "%s" , message );

        write( socket_to_uart->connect_fd , message , strlen(message));

        socket2uart_closeForced( socket_to_uart );
        return 0;
    }

    return -1;
}

void *relay_uart_to_socket( void *param )
{
    Socket2Uart *socket_to_uart=(Socket2Uart *)param;

    int read_size;
    uint8_t buffer[BUF_SIZE];
    int write_size;
    pthread_detach( pthread_self() );


    while(1)
    {
        pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );
        pthread_cond_wait( &socket_to_uart->condSocket2Uart , &socket_to_uart->mutexSocket2Uart );
        pthread_mutex_unlock( &socket_to_uart->mutexSocket2Uart );

        if ( socket_to_uart->connect_fd == -1 )
            continue;

        while( socket_to_uart->connect_fd >= 0)
        {
            read_size = CPAP_recv( 0 , buffer , sizeof(buffer) );

            if ( read_size > 0 )
            {
                if ( debug )
                {
                    char message[32];
                    sprintf( message , "socket[%d] <<< " , socket_to_uart->connect_fd );
                    printData( (char *)buffer , read_size , message , 1  );
                }

                write_size = write( socket_to_uart->connect_fd , buffer , read_size );

                if ( write_size < 0 )
                {
                    socket2uart_CloseClient( socket_to_uart );
                    printf_debug( "write to socket error\n" );
                    break;
                }
            }
            else if ( read_size < 0 )
            {
                char *ret_message="FAILED\nrs232 read error\n";
                write( socket_to_uart->connect_fd , ret_message , strlen(ret_message) );
                break;
            }
            else
                printf_debug( "read_size=0\n" );
/*
            if ( DisconnectIfNoPermit( socket_to_uart ) == 0 )
            {
                printf_debug( "relay too long\n" );
                break;
            }*/
        }

        if ( read_size <= 0 )
        {
            printf_debug( "relay uart to socket failed, read_size=%d\n" , read_size );
        }
    }

    socket_to_uart->relay_thread_handle = -1;
    return 0;
}


int socket2uart( Socket2Uart *socket_to_uart )
{
    char buffer[BUF_SIZE];

    socket_to_uart->relay_thread_handle = -1;
    socket_to_uart->listen_fd = -1;
    socket_to_uart->connect_fd = -1;
    pthread_cond_init( &socket_to_uart->condSocket2Uart , 0 );
    pthread_mutex_init( &socket_to_uart->mutexSocket2Uart , 0);

    while(1)
    {
        socket_to_uart->connect_fd = start_tcp_server( &socket_to_uart->listen_fd , socket_to_uart->port );

        if ( socket_to_uart->relay_thread_handle == -1 )
            pthread_create( &socket_to_uart->relay_thread_handle , 0 ,  relay_uart_to_socket , socket_to_uart );

        if ( DisconnectIfNoPermit( socket_to_uart ) == 0 )
        {
            printf_debug( "no permit, maybe ExecuteSeriesCommand take long time\n" );
            continue;
        }

        if ( socket_to_uart->connect_fd < 0 )
        {
            printf_error( "the listen socket error\n" );
            continue;
        }

        socket2uart_SetReconnected( socket_to_uart );

        do{
            if ( DisconnectIfNoPermit( socket_to_uart ) == 0 )
            {
                printf_debug( "socket2uart too long\n" );
                break;
            }

            int read_size;
            read_size = read_socket( socket_to_uart->connect_fd , buffer );

            if ( read_size < 0 )
                break;
            else if ( read_size == 0 )
            {
                printf_debug( "read socket[%d] time out\n" , socket_to_uart->connect_fd );
                break;
            }

            if ( debug )
            {
                printf_debug( "socket[%d] >>>\n" , socket_to_uart->connect_fd );
                printData( buffer , read_size , "" , 1  );
            }

            if ( CPAP_send( 0 , buffer , read_size ) >= 0 )
            {
                //note the uart-to-socket thread to read uart
                pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );
                pthread_cond_signal( &socket_to_uart->condSocket2Uart );
                pthread_mutex_unlock( &socket_to_uart->mutexSocket2Uart );
            }
            else
            {
                char *uart_open_failed="FAILED\nCPAP open error\n";
                write( socket_to_uart->connect_fd , uart_open_failed , strlen(uart_open_failed) );
            }

        }while(1);

        socket2uart_CloseClient( socket_to_uart );
    }
}
#if 0
static Socket2Uart *socket_to_uart_standalone;


static void handler(int sig)
{
    pid_t pid;

    pid = wait(NULL);

    printf("Pid %d exited.\n", pid);

    if (!fork())
    {
        printf("Child pid is %d\n", getpid());
        socket2uart( socket_to_uart_standalone );
    }
}

int main( int argc , char **argv )
{
    socket_to_uart_standalone = malloc( sizeof( Socket2Uart ) );
    if ( access( "/etc/debug" , 0 ) == 0 )
        debug=1;

    if ( argc == 3)
    {
        socket_to_uart_standalone->port = atoi( argv[1] );
        strncpy( socket_to_uart_standalone->uart_name , argv[2] , sizeof( socket_to_uart_standalone->uart_name) );
    }
    else
    {
        printf("example:socket2uart 21 /dev/ttyUSB0\n");
        exit(1);
    }
    printf_debug("listen port:%d\n",port);
    /* Variable and structure definitions. */

    signal(SIGCHLD, handler);
    if (!fork())
    {
        return socket2uart( socket_to_uart_standalone->uart_name , socket_to_uart_standalone->port );
    }

    //father process sleep and wait SIGCHLD
    while(1) sleep(1);

}
#endif
