


/************tcpclient.c************************/
/* Header files needed to use the sockets API. */
/* File contains Macro, Data Type and */
/* Structure definitions along with Function */
/* prototypes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include "common.h"

/* BufferLength is 100 bytes */
#define BufferLength 100
/* Default host name of server system. Change it to your default */
/* server hostname or IP.  If the user do not supply the hostname */
/* as an argument, the_server_name_or_IP will be used as default*/
#define SERVER "The_server_name_or_IP"


int TCP_Read( int descriptor , char *data , int size )
{
    int		err;
    fd_set	fdest;
    struct	timeval timeout;

    FD_ZERO(&fdest);
    FD_SET(descriptor, &fdest);

    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    if ( descriptor != -1 )
    {
        int Retry=100;
        do{
            err = select(descriptor+1, &fdest , NULL , NULL, &timeout );

            if (err > 0 )
            {
                if ( FD_ISSET( descriptor, &fdest ) )
                {
                    err = read( descriptor , data , size );
                    if ( err < 0 )
                    {
                        if (errno != EINTR && errno != EAGAIN )
                            perror( "socket read error" );
                    }
                    return err;
                }
                else
                    printf_debug("FD_ISSET=0\n");
            }
            else if ( err < 0 )
            {
                perror("recv");
                printf_debug("read error\n");
                if ( Retry-- <= 0 )
                {
                    break;
                }
            }

        }while( err < 0 && errno == EINTR );

    }

    return 0;

}


int TCP_Write( int descriptor , char *write_buffer , int write_length )
{
    int ret;
    socklen_t length = sizeof(int);
    char temp;

    /* Send string to the server using */
    /* the write() function. */
    /*********************************************/
    /* Write() some string to the server. */

    ret = write(descriptor, write_buffer , write_length );

    if(ret < 0)
    {
        perror("Client-write() error");
        printf_error( "FD(%d) write error\n" , ret );
        ret = getsockopt(descriptor, SOL_SOCKET, SO_ERROR, &temp, &length);
        if(ret == 0)
        {
            /* Print out the asynchronously received error. */
            errno = temp;
            perror("SO_ERROR was");
        }
        close(descriptor);
        return -1;
    }

    return 0;

}



int TCP_ConnectToServer( char *server , int port )
{
    int descriptor;
    struct sockaddr_in serveraddr;
    struct hostent *hostp;
    int ret=0;
    /* The socket() function returns a socket */
    /* descriptor representing an endpoint. */
    /* The statement also identifies that the */
    /* INET (Internet Protocol) address family */
    /* with the TCP transport (SOCK_STREAM) */
    /* will be used for this socket. */
    /******************************************/
    /* get a socket descriptor */
    if((descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Client-socket() error");
        return -1;
    }
    else
        printf_debug("Client-socket() OK\n");

    memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);

    if((serveraddr.sin_addr.s_addr = inet_addr(server)) == (unsigned long)INADDR_NONE)
    {

        /* When passing the host name of the server as a */
        /* parameter to this program, use the gethostbyname() */
        /* function to retrieve the address of the host server. */
        /***************************************************/
        /* get host address */
        hostp = gethostbyname(server);
        if(hostp == (struct hostent *)NULL)
        {
            printf_debug("HOST NOT FOUND --> ");
            /* h_errno is usually defined */
            /* in netdb.h */
            printf_debug("h_errno = %d\n",h_errno);
            printf_debug("---This is a client program---\n");
            close(descriptor);
            return -1;
        }
        memcpy(&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
    }

    /* After the socket descriptor is received, the */
    /* connect() function is used to establish a */
    /* connection to the server. */
    /***********************************************/
    /* connect() to server. */
    if((ret = connect(descriptor, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
    {
        perror("Client-connect() error");
        close(descriptor);
        return -1;
    }
    else
        printf_debug("FD(%d) Connection established...\n" , descriptor );

    return descriptor;

}

#if 0
/* Pass in 1 parameter which is either the */
/* address or host name of the server, or */
/* set the server name in the #define SERVER ... */
int main(int argc, char *argv[])
{
    /* Variable and structure definitions. */
    int descriptor, ret;

    unsigned char buffer[BufferLength];
    char server[255];

    if ( argc == 3)
    {
        strncpy( server , argv[1] , sizeof( server ) );
        port = atoi( argv[2] );
    }
    else
    {
        printf_debug("example:tcp_client localhost 21\n");
        exit(1);
    }

    int stdin_size;
    stdin_size = InputFromStdin( buffer , sizeof(buffer));

    if ( debug )
        printData( buffer , stdin_size , "send\n" , 1 );

    int descriptor;
    descriptor = TCP_ConnectToServer( server , port );

    TCP_Write( descriptor , buffer , stdin_size );

    int read_length;
    read_length = TCP_Read( descriptor , buffer , sizeof( buffer) );

    return 0;
}
#endif

