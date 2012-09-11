


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

#include "cpap.h"
/* BufferLength is 100 bytes */
#define BufferLength 100
/* Default host name of server system. Change it to your default */
/* server hostname or IP.  If the user do not supply the hostname */
/* as an argument, the_server_name_or_IP will be used as default*/
#define SERVER "The_server_name_or_IP"
/* Server's port number */
#define SERVPORT 9527

int port=SERVPORT;

int debug=0;

int getCmdFromStdin( char *cmdBuffer , int bufferSize )
{
    int intputCount=0;
    char inputFromStdIn;


    do
    {
        inputFromStdIn = getchar();

    //    printf_debug( "get 0x%x\n" , inputFromStdIn );

        if ( inputFromStdIn == '\n' ) break;

        cmdBuffer[intputCount++]=inputFromStdIn;


        if (intputCount >= bufferSize )
        {
            printf_debug("input too long,should be less then %d\n" , sizeof(cmdBuffer ));
            exit(1);
        }
    }while( inputFromStdIn != '\n' );

    return intputCount;
}



/* Pass in 1 parameter which is either the */
/* address or host name of the server, or */
/* set the server name in the #define SERVER ... */
int main(int argc, char *argv[])
{
    if ( access( "/etc/debug" , 0 ) == 0 )
        debug=1;

    /* Variable and structure definitions. */
    int sd, rc, length = sizeof(int);
    struct sockaddr_in serveraddr;
    unsigned char buffer[BufferLength];
    char server[255];
    char temp;
    int totalcnt = 0;
    struct hostent *hostp;

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
    stdin_size = getCmdFromStdin( buffer , sizeof(buffer));

    if ( debug )
        printData( buffer , stdin_size , "send\n");

    /* The socket() function returns a socket */
    /* descriptor representing an endpoint. */
    /* The statement also identifies that the */
    /* INET (Internet Protocol) address family */
    /* with the TCP transport (SOCK_STREAM) */
    /* will be used for this socket. */
    /******************************************/
    /* get a socket descriptor */
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Client-socket() error");
        exit(-1);
    }
    else
        printf_debug("Client-socket() OK\n");

    memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SERVPORT);

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
            printf_debug("Command usage: %s <server name or IP>\n", argv[0]);
            close(sd);
            exit(-1);
        }
        memcpy(&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));
    }

    /* After the socket descriptor is received, the */
    /* connect() function is used to establish a */
    /* connection to the server. */
    /***********************************************/
    /* connect() to server. */
    if((rc = connect(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
    {
        perror("Client-connect() error");
        close(sd);
        exit(-1);
    }
    else
        printf_debug("Connection established...\n");

    /* Send string to the server using */
    /* the write() function. */
    /*********************************************/
    /* Write() some string to the server. */

    rc = write(sd, buffer , stdin_size );

    if(rc < 0)
    {
        perror("Client-write() error");
        rc = getsockopt(sd, SOL_SOCKET, SO_ERROR, &temp, &length);
        if(rc == 0)
        {
            /* Print out the asynchronously received error. */
            errno = temp;
            perror("SO_ERROR was");
        }
        close(sd);
        exit(-1);
    }
    else
    {
        printf_debug("Waiting the %s to echo back...\n", server);
    }


//    exit(0);
    char recv_buf[1024];
    int recv_size;
    do
    {
        recv_size = recvCPAPResponse( sd, recv_buf , sizeof(recv_buf ) , buffer[1] , 5 );
        if ( recv_size <= 0 )
            write(sd, buffer , stdin_size );
    }
    while( recv_size  <= 0 );

    printData( recv_buf , recv_size , "" );
    return 0;
}
