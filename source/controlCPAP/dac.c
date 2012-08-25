#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dac.h>
#include <stdint.h>
#include <common.h>


#include <sys/ioctl.h>


#include <linux/i2c.h>
#include <linux/i2c-dev.h>



static int dacDesc[2];
static const uint16_t address[MAX_CHANNEL]=
{
    0x1e18,
    0x1e19,
    0x1e1a,
    0x1e1b,
    0x1b18,
    0x1b19
};

static int openDAC( int ch)
{
    char node[128];
    int dacDescriptor;
    bzero( node , sizeof(node));
    // Open i2c-1 device
    sprintf(node, "/dev/i2c-%d", ch);
    if( (dacDescriptor = open(node, O_RDWR) ) < 0 )
    {
        printf_error("Cannot Open I2C Master Device\n");
        return -1;
    }

    return dacDescriptor;

}

int initDAC( void )
{
    if ( openDAC(1) < 0 )
        return -1;
    dacDesc[0] = openDAC(1);

    return 0;
}

#define HIGH_BYTE( word ) \
    ( word >> 8 & 0xff )
#define LOW_BYTE( word ) \
    ( word & 0xff )

int writeDAC( int channel , uint16_t adjustedValue )
{
    char i2c_ptr[3];

    if ( debug ) printf( "0x%x," , adjustedValue );
    // write data operation
    if( ioctl( dacDesc[0]  , I2C_SLAVE_FORCE, HIGH_BYTE( address[channel] ) ) < 0 )
    {
       printf_error("Cannot set I2C address : desc=%d 0x%x\n" , dacDesc[0] , HIGH_BYTE( address[channel] ) );
       perror( "ioctl" );
       return (-1);
    }

    i2c_ptr[1] = ( adjustedValue >> 8) & 0xff;
    i2c_ptr[2] = ( adjustedValue & 0xff );
    i2c_ptr[0] = LOW_BYTE( address[channel] ) ;
    if( write( dacDesc[0] , i2c_ptr , 3 ) != 3)
    {
        printf_error("I2C fd(%d) write failed\n" , dacDesc[0] );
        perror( "write" );
        return (-1);
    }

    return 0;
}
