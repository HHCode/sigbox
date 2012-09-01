#include "tcp_server.h"
#include "rs232.h"
#include <stdlib.h>
#include "stdio.h"
#include "string.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#define BUF_SIZE 1024
#define SERVPORT 9527



int debug=0;
int port=SERVPORT;
char uart_name[32];
int uart_fd=-1;
int listen_fd, connect_fd;

#ifndef printf_debug
#define printf_debug(fmt, args...)\
{\
    if ( debug ) printf("%s[%d]: "fmt, __FILE__, __LINE__, ##args);\
}
#endif

#ifndef printf_error
#define printf_error(fmt, args...)\
{\
    fprintf(stderr, "FAILED\n" ); \
    fprintf(stderr, "%s[%d]: "fmt, __FILE__, __LINE__, ##args); \
}
#endif

pthread_t relay_thread_handle;
void *relay_uart_to_socket( void *param )
{
    int read_size;
    char buffer[BUF_SIZE];
    int write_size;
    pthread_detach( pthread_self() );
    while(1)
    {
        read_size = rs232_recv( uart_fd , buffer , sizeof(buffer) );

        if ( debug )
        {
            printf("from rs232,%d:\n" , read_size );
            print_data( (unsigned char *)buffer , read_size );
            printf("\n");
        }

        if ( read_size > 0 )
        {
            write_size = write( connect_fd , buffer , read_size );

            if ( write_size < 0 )
            {
                printf_error( "write to socket error\n" );
                break;
            }
        }
        else if ( read_size < 0 )
        {
            perror( "rs232_read");
            printf_error( "read from uart error\n"  );
            break;
        }
    }
    return 0;
}


int socket2uart( char *uart_name , int port )
{
    if ( access( "/etc/debug" , 0 ) == 0 )
        debug=1;

    char buffer[BUF_SIZE];

    while(1)
    {
        while( uart_fd < 0 )
        {
            uart_fd = rs232_open( uart_name , 9600 );
            if ( uart_fd > 0 )
                break;
            printf_error( "open %s error\n" , uart_name );
            sleep(1);
        }

        connect_fd = start_tcp_server( &listen_fd , port );


        pthread_create( &relay_thread_handle , 0 ,  relay_uart_to_socket , 0 );

        if ( connect_fd < 0 )
            exit(1);

        int read_size;

        do{
            bzero( buffer , sizeof(buffer));
            read_size = read_socket( connect_fd , buffer );

            if ( read_size < 0 )
                break;

            if ( debug )
            {
                printf("from socket,%d:\n" , read_size );
                print_data( ( unsigned char *)buffer , read_size );
                printf("\n");
            }

            if ( uart_fd >= 0 )
            {
                if ( rs232_write( uart_fd , buffer , read_size ) < 0 )
                {
                    perror( "rs232_write" );
                    printf_error( "write to %s error\n" , uart_name );
                    continue;
                }
            }

        }while(1);

        close( connect_fd );
        close( listen_fd );
        close( uart_fd );
        uart_fd=-1;
    }
}


void handler(int sig)
{
    pid_t pid;

    pid = wait(NULL);

    printf("Pid %d exited.\n", pid);

    if (!fork())
    {
        printf("Child pid is %d\n", getpid());
        socket2uart( uart_name , port );
    }
}

int main( int argc , char **argv )
{
    if ( access( "/etc/debug" , 0 ) == 0 )
        debug=1;

    if ( argc == 3)
    {
        port = atoi( argv[1] );
        strncpy( uart_name , argv[2] , sizeof( uart_name) );
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
        return socket2uart( uart_name , port );
    }

    //father process sleep and wait SIGCHLD
    while(1) sleep(1);

}
