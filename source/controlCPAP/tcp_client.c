


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
/* BufferLength is 100 bytes */
#define BufferLength 100
/* Default host name of server system. Change it to your default */
/* server hostname or IP.  If the user do not supply the hostname */
/* as an argument, the_server_name_or_IP will be used as default*/
#define SERVER "The_server_name_or_IP"
/* Server's port number */
#define SERVPORT 9527

int port=SERVPORT;


void printData( char *data , int size , char *prefix )
{
    char printBuf[2048];

    printf( "%s" , prefix );
    int index;
    int stringLength=0;
    int binaryCount=0;
    bzero( printBuf , sizeof(printBuf) );
    for( index=0 ; index<size ; index++ )
    {
        unsigned char *testedArray=( char * )data;
        if ( isprint( testedArray[index] ) )
        {
            printBuf[stringLength++] = testedArray[index];
        }
        else
        {
            if ( testedArray[index] == 0x0d )
                stringLength += sprintf( &printBuf[stringLength] , "\\r," );
            else if ( testedArray[index] == 0x0a )
                stringLength += sprintf( &printBuf[stringLength] , "\\n," );
            else
            {
                stringLength += sprintf( &printBuf[stringLength] , "0x%x," , testedArray[index] );
                binaryCount++;
            }
        }
#if 0
        if ( binaryCount > 10 )
        {
            strcpy( &printBuf[stringLength] , "..." );
            break;
        }
#endif
    }
    printf( "%s\n" , printBuf );
}


int getCmdFromStdin( char *cmdBuffer , int bufferSize )
{
    int intputCount=0;
    char inputFromStdIn;


    do
    {
        inputFromStdIn = getchar();

    //    printf( "get 0x%x\n" , inputFromStdIn );

        if ( inputFromStdIn == '\n' ) break;

        cmdBuffer[intputCount++]=inputFromStdIn;


        if (intputCount >= bufferSize )
        {
            printf("input too long,should be less then %d\n" , sizeof(cmdBuffer ));
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
    /* Variable and structure definitions. */
    int sd, rc, length = sizeof(int);
    struct sockaddr_in serveraddr;
    char buffer[BufferLength];
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
        printf("example:tcp_client localhost 21\n");
        exit(1);
    }

    int stdin_size;
    stdin_size = getCmdFromStdin( buffer , sizeof(buffer));

    printData( buffer , stdin_size , "<<<<\n");

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
        printf("Client-socket() OK\n");

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
            printf("HOST NOT FOUND --> ");
            /* h_errno is usually defined */
            /* in netdb.h */
            printf("h_errno = %d\n",h_errno);
            printf("---This is a client program---\n");
            printf("Command usage: %s <server name or IP>\n", argv[0]);
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
        printf("Connection established...\n");

    /* Send string to the server using */
    /* the write() function. */
    /*********************************************/
    /* Write() some string to the server. */
    printf("Sending some string to the f***ing %s...\n", server);

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
        printf("Waiting the %s to echo back...\n", server);
    }


//    exit(0);

    totalcnt = 0;
    while(totalcnt < 7 )
    {

        /* Wait for the server to echo the */
        /* string by using the read() function. */
        /***************************************/
        /* Read data from the server. */
        rc = read(sd, &buffer[totalcnt], BufferLength-totalcnt);
        if(rc < 0)
        {
            perror("Client-read() error");
            close(sd);
            exit(-1);
        }
        else if (rc == 0)
        {
            printf("Server program has issued a close()\n");
            close(sd);
            exit(-1);
        }
        else
            totalcnt += rc;
    }
    printf("Client-read() is OK\n");
    //printf("Echoed data from the f***ing server: %s\n", buffer);
    printData( buffer , totalcnt , ">>>>>\n");
    /* When the data has been read, close() */
    /* the socket descriptor. */
    /****************************************/
    /* Close socket descriptor from client side. */
    close(sd);
    exit(0);
    return 0;
}
