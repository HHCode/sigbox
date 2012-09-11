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
#define BUF_SIZE 1024
#define SERVPORT 9527



void *functionSocket2Uart( void *param )
{
    Socket2Uart *socket_to_uart=(Socket2Uart *)param;

    socket2uart( socket_to_uart );
    return 0;
}



void socket2uartRefreshUART( Socket2Uart *socket_to_uart , int uart_descriptor )
{
    socket_to_uart->uart_fd = uart_descriptor;
}

void Init_socket2uart( Socket2Uart *socket_to_uart , int uart_descriptor )
{
    socket_to_uart->uart_fd = uart_descriptor;
    pthread_create( &socket_to_uart->threadSocket2Uart , 0 , functionSocket2Uart , socket_to_uart );
}

int socket2uart_IsConnect( Socket2Uart *socket_to_uart )
{
    if ( socket_to_uart->connect_fd != -1 )
        return 1;
    else
        return 0;
}


void *relay_uart_to_socket( void *param )
{
    Socket2Uart *socket_to_uart=(Socket2Uart *)param;

    int read_size;
    char buffer[BUF_SIZE];
    int write_size;
    pthread_detach( pthread_self() );


    while(1)
    {
        pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );
        pthread_cond_wait( &socket_to_uart->condSocket2Uart , &socket_to_uart->mutexSocket2Uart );
        pthread_mutex_unlock( &socket_to_uart->mutexSocket2Uart );

        if ( socket_to_uart->connect_fd == -1 )
            continue;

        if ( socket_to_uart->uart_fd < 0 )
        {
            char *ret_message="CPAP not ready\n";
            write( socket_to_uart->connect_fd , ret_message , strlen(ret_message) );
            printf_debug( "%s\n" , ret_message );
            continue;
        }

        printf_debug( "note to read uart[%d]\n" , socket_to_uart->uart_fd );

        while( socket_to_uart->connect_fd >= 0)
        {
            read_size = rs232_recv( socket_to_uart->uart_fd , buffer , sizeof(buffer) );
            if ( read_size > 0 )
            {
                if ( debug )
                {
                    printf_debug( "socket[%d] <<< uart[%d]\n" , socket_to_uart->connect_fd , socket_to_uart->uart_fd );
                    printData( buffer , read_size , "" );
                }

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
                perror( "rs232_read");
                printf_error( "read from uart error\n"  );
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
                printf_debug( "socket[%d] >>> uart[%d]\n" , socket_to_uart->connect_fd , socket_to_uart->uart_fd );
                printData( buffer , read_size , "" );
            }

            if ( socket_to_uart->uart_fd < 0 )
            {
                char *ret_message="CPAP not ready\n";
                write( socket_to_uart->connect_fd , ret_message , strlen(ret_message) );
                printf_debug( "%s\n" , ret_message );
                continue;
            }

            //empty the rs232 data
            if ( socket_to_uart->uart_fd >= 0 )
                while( rs232_recv( socket_to_uart->uart_fd , buffer , sizeof(buffer) ) > 0 );
            else
                printf_error( "uart[%d] open failed\n" , socket_to_uart->uart_fd );

            if ( socket_to_uart->uart_fd >= 0 && rs232_write( socket_to_uart->uart_fd , buffer , read_size ) >= 0 )
            {
                //note the uart-to-socket thread to read uart
                pthread_mutex_lock( &socket_to_uart->mutexSocket2Uart );
                pthread_cond_signal( &socket_to_uart->condSocket2Uart );
                pthread_mutex_unlock( &socket_to_uart->mutexSocket2Uart );
            }
            else
            {
                perror( "rs232_write" );
                printf_error( "fd:%d,write to %s error\n" ,  socket_to_uart->uart_fd, socket_to_uart->uart_name );
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
