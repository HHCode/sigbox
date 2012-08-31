#include "tcp_server.h"
#include "rs232.h"
#include <stdlib.h>
#include "stdio.h"
#include "string.h"



int debug=0;

#define BUF_SIZE 1024
#define SERVPORT 9527


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

int main()
{
    if ( access( "/etc/debug" , 0 ) == 0 )
        debug=1;

    /* Variable and structure definitions. */
    int listen_fd, connect_fd;

    char buffer[BUF_SIZE];

    while(1)
    {
        connect_fd = start_tcp_server( &listen_fd , SERVPORT );

        if ( connect_fd < 0 )
            exit(1);

        int read_size;
        int write_size;
        int uart_fd;

        uart_fd = rs232_open( "/dev/ttyS0" , 9600 );


        do{
            bzero( buffer , sizeof(buffer));
            read_size = read_socket( connect_fd , buffer );

            if ( read_size < 0 )
                break;

            printf("%s", buffer);
            rs232_write( uart_fd , buffer , read_size );

            read_size = rs232_recv( uart_fd , buffer , read_size );

            if ( read_size < 0 )
            {
                perror( "rs232_read");
                printf_error( "read from uart error\n"  );
                break;
            }

            if ( read_size > 0 )
                write_size = write( connect_fd , buffer , read_size );

            if ( write_size < 0 )
            {
                printf_error( "write to socket error\n" );
                break;
            }
        }while(1);

        close( connect_fd );
        close( listen_fd );
    }while(1);

}
