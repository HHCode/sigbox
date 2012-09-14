/************tcpserver.c************************/
/* header files needed to use the sockets API */
/* File contain Macro, Data Type and Structure */
/***********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

/* BUF_SIZE is 100 bytes */
#define BUF_SIZE 100
/* Server port number */
#define SERVPORT 3111




extern int debug;

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


int read_socket( int connect_fd , char *buffer )
{
    int ret;
    fd_set read_fdset;
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    /* The select() function allows the process to */
    /* wait for an event to occur and to wake up */
    /* the process when the event occurs. In this */
    /* example, the system notifies the process */
    /* only when data is available to read. */
    /***********************************************/
    /* Wait for up to 15 seconds on */
    /* select() for data to be read. */
    FD_ZERO(&read_fdset);
    FD_SET(connect_fd, &read_fdset);
    ret = select(connect_fd+1, &read_fdset, NULL, NULL, &timeout);
    if((ret == 1) && (FD_ISSET(connect_fd, &read_fdset)))
    {
        /* Read data from the client. */
        /* When select() indicates that there is data */
        /* available, use the read() function to read */
        /* 100 bytes of the string that the */
        /* client sent. */
        /***********************************************/
        /* read() from client */
        ret = read( connect_fd, buffer , BUF_SIZE );
        if(ret < 0)
        {
            perror("Server-read() error");
            return -1;
        }
        else if (ret == 0)
        {
            printf_debug("Client program has issued a close()\n");
            return -1;
        }
        else
        {
            printf_debug("Server-read() is OK\n");
        }
    }
    else if (ret < 0)
    {
        perror("Server-select() error");
        return -1;
    }
    /* ret == 0 */
    else
    {
        printf_debug("Server-select() timed out.\n");
        return 0;
    }

    return ret;
}

int start_tcp_server( int *listen_fd , int port )
{
    int ret;
    int connect_fd;
    int on = 1;
    struct sockaddr_in serveraddr;
    struct sockaddr_in their_addr;

    if ( *listen_fd == -1 )
    {
        /* The socket() function returns a socket descriptor */
        /* representing an endpoint. The statement also */
        /* identifies that the INET (Internet Protocol) */
        /* address family with the TCP transport (SOCK_STREAM) */
        /* will be used for this socket. */
        /************************************************/
        /* Get a socket descriptor */
        if((*listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Server-socket() error");
            /* Just exit */
            return -1;
        }
        else
            printf_debug("Server-socket() is OK\n");

        /* The setsockopt() function is used to allow */
        /* the local address to be reused when the server */
        /* is restarted before the required wait time */
        /* expires. */
        /***********************************************/
        /* Allow socket descriptor to be reusable */
        if((ret = setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on))) < 0)
        {
            perror("Server-setsockopt() error");
            close(*listen_fd);
            return -1;
        }
        else
            printf_debug("Server-setsockopt() is OK\n");

        /* bind to an address */
        memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_port = htons(port);
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

        printf_debug( "Using %s, listening at %d\n", (char *)inet_ntoa(serveraddr.sin_addr), port );

        /* After the socket descriptor is created, a bind() */
        /* function gets a unique name for the socket. */
        /* In this example, the user sets the */
        /* s_addr to zero, which allows the system to */
        /* connect to any client that used port 3005. */
        if((ret = bind(*listen_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
        {
            perror("Server-bind() error");
            /* Close the socket descriptor */
            close(*listen_fd);
            /* and just exit */
            return -1;
        }
        else
            printf_debug("Server-bind() is OK\n");

        /* The listen() function allows the server to accept */
        /* incoming client connections. In this example, */
        /* the backlog is set to 10. This means that the */
        /* system can queue up to 10 connection requests before */
        /* the system starts rejecting incoming requests.*/
        /*************************************************/
        /* Up to 10 clients can be queued */
        if((ret = listen(*listen_fd, 10)) < 0)
        {
            perror("Server-listen() error");
            close(*listen_fd);
            return -1;
        }
        else
            printf_debug("Server-Ready for client connection...\n");
    }

    /* The server will accept a connection request */
    /* with this accept() function, provided the */
    /* connection request does the following: */
    /* - Is part of the same address family */
    /* - Uses streams sockets (TCP) */
    /* - Attempts to connect to the specified port */
    /***********************************************/
    /* accept() the incoming connection request. */
    socklen_t sin_size = sizeof(struct sockaddr_in);
    if((connect_fd = accept(*listen_fd, (struct sockaddr *)&their_addr, &sin_size)) < 0)
    {
        perror("Server-accept() error");
        close(*listen_fd);
        return -1;
    }
    else
        printf_debug("Server-accept() is OK\n");

    /*client IP*/
    printf_debug("Got connection from client: %s\n", (char *)inet_ntoa(their_addr.sin_addr));

    return connect_fd;
}


#if 0
int main()
{
    /* Variable and structure definitions. */
    int listen_fd, *connect_fd, ret, length = sizeof(int);

    char temp;
    char buffer[BUF_SIZE];

    *connect_fd = start_tcp_server( &listen_fd , SERVPORT );

    if ( *connect_fd < 0 )
        exit(1);

    int read_size;
    int uart_fd;



    do{
        read_size = read_socket( *connect_fd , buffer );

        write( 1 , buffer , read_size );
        if ( read_size < 0 )
        {
            exit(1);
        }

    }while(1);


    /* Echo some bytes of string, back */
    /* to the client by using the write() */
    /* function. */
    /************************************/
    /* write() some bytes of string, */
    /* back to the client. */
    int totalcnt=read_size;
    printf_debug("Server-Echoing back to client...\n");
    ret = write(*connect_fd, buffer, totalcnt);
    if(ret != totalcnt)
    {
        perror("Server-write() error");
        /* Get the error number. */
        ret = getsockopt(*connect_fd, SOL_SOCKET, SO_ERROR, &temp, &length);
        if(ret == 0)
        {
            /* Print out the asynchronously */
            /* received error. */
            errno = temp;
            perror("SO_ERROR was: ");
        }
        else
            printf_debug("Server-write() is OK\n");

        close(listen_fd);
        close(*connect_fd);
        return -1;
    }

    /* When the data has been sent, close() */
    /* the socket descriptor that was returned */
    /* from the accept() verb and close() the */
    /* original socket descriptor. */
    /*****************************************/
    /* Close the connection to the client and */
    /* close the server listening socket. */
    /******************************************/
    close(*connect_fd);
    close(listen_fd);
    exit(0);
    return 0;
}
#endif
