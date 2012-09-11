#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <ctype.h>

#include "rs232.h"
#include "sys/ioctl.h"


#ifdef DEBUG
#define printf_debug(fmt, args...) printf("%s[%d]: "fmt, __FUNCTION__, __LINE__, ##args)
#else
#define printf_debug(fmt, args...)
#endif


/* Global data: */
static struct termios   rs232_attr;

#ifdef DEBUG
static	char rs232_dbg='Y';
#else
static	char rs232_dbg='N';
#endif


void print_data( unsigned char *data , int size )
{
    int i;
    for( i=0 ; i<size ; i++ )
    {
//        if ( isalpha( data[i] ) )
//            printf("%c",data[i]);
//        else
        printf("0x%X," , data[i] );
    }
}

/**
 * @brief rs232_recv
 *
 * @param[in]   handle
 *      The opened file descriptor
 *
 * @param[in]   scheduleStatus
 *      User is able to set schedule to specify the night mode in a duration
 *
 * @author  HawkHsieh
 *
 * @date    2012/06/22
 * @retval 0    ok
 * @retval -1   failed
*/
int rs232_recv( int handle , char *data , int size )
{
	int		err;
	fd_set	fdest;	
	struct	timeval timeout;

	FD_ZERO(&fdest);
    FD_SET(handle, &fdest);

    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;

    if ( handle != -1 )
	{
        int Retry=100;
		do{
            err = select(handle+1, &fdest , NULL , NULL, &timeout );

			if (err > 0 )
			{
                if ( FD_ISSET( handle, &fdest ) )
				{
                    err = read( handle , data , size );
					if ( err < 0 )
					{
						if (errno != EINTR && errno != EAGAIN )
							perror( "error when read rs232\n" );
					}

					if ( rs232_dbg == 'Y' )
					{
						if ( err > 0 )
						{
                            printf("recv: ");
                            print_data( (unsigned char *)data , err );
                            printf("\n");
						}
					
					}
					return err;
				}
				else
					printf("FD_ISSET=0\n");
            }
            else if ( err < 0 )
			{
				perror("rs232_recv");
                printf_debug("read error\n");
				if ( Retry-- <= 0 )
				{
                    printf_debug("Abort rs232_recv due to retry 100 times error\n");
					break;
				}
            }

		}while( err < 0 && errno == EINTR );

	}

	return 0;
}



/**
 * @brief rs232_recv
 *
 * @param[in]   handle
 *      The opened file descriptor
 *
 * @param[in]   scheduleStatus
 *      User is able to set schedule to specify the night mode in a duration
 *
 * @author  HawkHsieh
 *
 * @date    2012/06/22
 * @retval 0    ok
 * @retval -1   failed
*/
int rs232_SetBaudrate( int handle, int nBaudrate )
{
    long    BAUD;

    if((tcgetattr (handle, &rs232_attr)) < 0)
    {
        perror("tcgetattr\n");
        printf_debug("tcgetattr error\n");
        return -1;
    }

    // Baud rate
    switch(nBaudrate)
    {
        case 115200:
            BAUD = B115200;
            break;

        case 57600:
            BAUD = B57600;
            break;

        case 38400:
            BAUD = B38400;
            break;

        case 19200:
            BAUD = B19200;
            break;

        case 9600:
        default:
            BAUD = B9600;
            break;

        case 4800:
            BAUD = B4800;
            break;

        case 2400:
            BAUD = B2400;
            break;

        case 1800:
            BAUD = B1800;
            break;

        case 1200:
            BAUD = B1200;
            break;
    }   // end of switch baud_rate

    // char("Baud Rate:%d\n",nBaudrate);
    rs232_attr.c_cflag = BAUD;

    // parity none
    rs232_attr.c_cflag |= CS8;

    // parity even rs232_attr.c_cflag = CS7 | PARENB;
    // parity odd rs232_attr.c_cflag = CS7 | PARODD;
    // ;
    // enable receiver,
    rs232_attr.c_cflag |= CREAD;

    // lower modem lines on last close 1 stop bit (since CSTOPB off)
    rs232_attr.c_cflag |= HUPCL;

    // ignore modem status lines
    rs232_attr.c_cflag |= CLOCAL;

    // turn off all output processing
    rs232_attr.c_oflag = 0;

    // Xon/Xoff flow control
//    rs232_attr.c_iflag = IXON | IXOFF;		//marked by hawk, since the char: 0x13 0x11 cant be received when IXON is set.
    rs232_attr.c_iflag = IXOFF;

    // ignore breaks
    rs232_attr.c_iflag |= IGNBRK;

    // ignore input parity errors
    rs232_attr.c_iflag |= IGNPAR;

    // everything off in local flag: disables canonical mode disables signal
    // generation disables echo
    rs232_attr.c_lflag = 0;

    cfsetispeed (&rs232_attr, nBaudrate);
    cfsetospeed (&rs232_attr, nBaudrate);

    if((tcsetattr (handle, TCSANOW, &rs232_attr)) < 0)
	{
		perror("tcsetattr");
        printf_debug("tcsetattr error \n");
		return -1;
	}
	return 0;
}

/**
 * @brief       rs232_open
 * @attention   no attention
 * @param[in]   devName
 *      the port number
 *
 * @param[in]   nBaudrate
 *      The Baudrate
 *
 * @author      HawkHsieh
 *
 * @date    2012/06/22
 * @retval 0    ok
 * @retval -1   failed
*/
int rs232_open( char *devName , int nBaudrate )
{
    int handle;

    if((handle = open ( devName , O_RDWR | O_SYNC )) < 0)
    {
        printf_debug("open error\n");
        return -1;
    }

	if ( rs232_SetBaudrate( handle , nBaudrate ) < 0 )
	{
        printf_debug("Set baud rate error\n");
		close(handle);
		handle = -1;
	}
	else
	{
		if ( rs232_dbg == 'Y' )
		{
            printf("open %s , baud=%d , handle=%d\n", devName , nBaudrate , handle);
		}
	}

    return handle;
}


/**
 * @brief       rs232_close
 * @attention   no attention
 * @param[in]   handle
 *      the port number
 *
 * @author      HawkHsieh
 *
 * @date    2012/06/22
 * @retval 0    ok
 * @retval -1   failed
*/
int rs232_close( int handle )
{
	if ( rs232_dbg == 'Y' )
	{
        printf_debug("close rs232, handle=%d\n", handle);
	}
    return close (handle);
}

/**
 * @brief       rs232_close
 * @attention   no attention
 * @param[in]   handle
 *      device handle
 *
 * @param[in]   buffer
 *      The data you want read
 *
 * @param[in]   nLen
 *      The data length
 *
 * @author      HawkHsieh
 *
 * @date    2012/06/22
 * @retval >0   actually read size
 * @retval -1   failed
*/
int rs232_read(int handle, char *buffer, long nLen)
{
    memset (buffer, 0x0, nLen);

    return read (handle, buffer, nLen);
}


/**
 * @brief       rs232_write
 * @attention   no attention
 * @param[in]   handle
 *      device handle
 *
 * @param[in]   buffer
 *      The data you want write
 *
 * @param[in]   nLen
 *      The data length
 *
 * @author      HawkHsieh
 *
 * @date    2012/06/22
 * @retval >0   actually read size
 * @retval -1   failed
*/
int rs232_write(int handle, char *buffer, long nLen)
{
    if ( rs232_dbg == 'Y' )
        print_data( (unsigned char *)buffer , nLen );

    return write (handle, buffer, nLen);
}



/**
 * @brief       Return how many byte is waiting
 * @attention   no attention
 * @param[in]   handle
 *      device handle
 *
 * @author      HawkHsieh
 *
 * @date    2012/06/22
 * @retval >0   actually read size
 * @retval -1   failed
*/
int rs232_WaitingByte( int handle )
{
	int Size;
	int err;

	err = ioctl( handle , FIONREAD , &Size );
	
	if ( err < 0 ) perror("rs232_WaitingByte error\n");

    if ( rs232_dbg )
        printf("WaitingByte=%d\n",err);

	return Size;
}
