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



void *functionSocket2Uart( void *param )
{
    Socket2Uart *socket_to_uart=(Socket2Uart *)param;

    socket2uart( socket_to_uart );
    return 0;
}


void Init_socket2uart( Socket2Uart *socket_to_uart )
{
    pthread_create( &socket_to_uart->threadSocket2Uart , 0 , functionSocket2Uart , socket_to_uart );
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


void socket2uart_closeForced( Socket2Uart *socket_to_uart )
{
    pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );
    close( socket_to_uart->connect_fd );
    socket_to_uart->connect_fd=-1;
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
            printf_debug( "socket[%d] <<< uart\n" , socket_to_uart->connect_fd );

            read_size = CPAP_recv( 0 , buffer , sizeof(buffer)  );
            if ( read_size > 0 )
            {
                write_size = write( socket_to_uart->connect_fd , buffer , read_size );

                if ( write_size < 0 )
                {
                    if ( socket_to_uart->connect_fd >= 0 )
                        close ( socket_to_uart->connect_fd );
                    socket_to_uart->connect_fd = -1;
                    printf_error( "write to socket error\n" );
                    break;
                }
            }
            else if ( read_size < 0 )
            {
                char *ret_message="rs232 read error\n";
                write( socket_to_uart->connect_fd , ret_message , strlen(ret_message) );
                break;
            }
        }
    }

    return 0;
}

int socket2uart( Socket2Uart *socket_to_uart )
{
    char buffer[BUF_SIZE];

    socket_to_uart->relay_thread_handle = -1;
    socket_to_uart->listen_fd = -1;
    pthread_cond_init( &socket_to_uart->condSocket2Uart , 0 );
    pthread_mutex_init( &socket_to_uart->mutexSocket2Uart , 0);

    while(1)
    {
        socket_to_uart->connect_fd = -1;
        socket_to_uart->connect_fd = start_tcp_server( &socket_to_uart->listen_fd , socket_to_uart->port );

        int retry=20;
        //Wait permit from cpapd. about 0.5s will be timeout
        while( socket2uart_getExecutePermit( socket_to_uart ) == 0 && retry-- > 0 )
            usleep( 50000 );
        if ( retry <=0 )
        {
            socket2uart_closeForced( socket_to_uart );
            continue;
        }

        if ( socket_to_uart->relay_thread_handle == -1 )
            pthread_create( &socket_to_uart->relay_thread_handle , 0 ,  relay_uart_to_socket , socket_to_uart );

        if ( socket_to_uart->connect_fd < 0 )
            printf_error( "the listen socket error\n" );


        do{
#ifdef GENIUSTOM_PROTOCOL
            int read_size=0;
            int crlf=-1;
            int total_size=0;
            int cmd_start_index=-1;
            int buffer_index=0;
            char command[1024];

            bzero( buffer , sizeof(buffer));

            while( crlf == -1 )
            {
                read_size = read_socket( socket_to_uart->connect_fd , &buffer[total_size] );

                if ( read_size < 0 )
                    break;
                else if ( read_size == 0 )
                    continue;

                total_size+=read_size;


                if ( debug )
                {
                    printData( buffer , total_size , "socket >>>\n" );
                }

                if ( cmd_start_index == -1 )
                {
                    for( buffer_index=0 ; buffer_index<total_size ; buffer_index++ )
                    {
                        if ( buffer[buffer_index] == 0x93 )
                        {
                            cmd_start_index=buffer_index;
                            break;
                        }
                    }
                }

                for( buffer_index=0 ; buffer_index<total_size ; buffer_index++ )
                {
                    if ( buffer[buffer_index] == '\n' && buffer[buffer_index-1] == '\r' )
                    {
                        crlf=buffer_index-1;
                        break;
                    }
                }
            }

            int command_size=buffer_index-cmd_start_index;
            memcpy( command , &buffer[cmd_start_index] , command_size );
#else
            int read_size;
            read_size = read_socket( socket_to_uart->connect_fd , buffer );
#endif
            if ( read_size < 0 )
                break;
            else if ( read_size == 0 )
            {
                printf_debug( "read socket[%d] time out\n" , socket_to_uart->connect_fd );
                break;
            }

            if ( debug )
            {
                printf_debug( "socket[%d] >>> uart\n" , socket_to_uart->connect_fd );
                printData( buffer , read_size , "" );
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

        close( socket_to_uart->connect_fd );
        printf_debug( "close socket[%d]\n" , socket_to_uart->connect_fd );
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