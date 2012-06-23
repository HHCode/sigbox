#ifndef __RS232_H
#define __RS232_H

#include <termios.h>
#include <unistd.h>


#define BTN_UP                  0x01
#define BTN_DOWN                0x02
#define BTN_LEFT                0x04
#define BTN_RIGHT               0x08

#define BTN_ZOOM_WIDE           0x01
#define BTN_ZOOM_TELE           0x02
#define BTN_FOCUS_NEAR          0x04
#define BTN_FOCUS_FAR           0x08
#define BTN_AUTO_PAN            0x10
#define BTN_NEXT_CAMERA         0x20

#define rs232_baudrate_50       B50
#define rs232_baudrate_75       B75
#define rs232_baudrate_110      B110
#define rs232_baudrate_134      B134
#define rs232_baudrate_150      B150
#define rs232_baudrate_200      B200
#define rs232_baudrate_300      B300
#define rs232_baudrate_600      B600
#define rs232_baudrate_1200     B1200
#define rs232_baudrate_2400     B2400
#define rs232_baudrate_4800     B4800
#define rs232_baudrate_9600     B9600
#define rs232_baudrate_19200    B19200
#define rs232_baudrate_38400    B38400
#define rs232_baudrate_57600    B57600
#define rs232_baudrate_1152000  B115200
#define rs232_baudrate_230400   B230400

#define rs232_databits_5        CS5
#define rs232_databits_6        CS6
#define rs232_databits_7        CS7
#define rs232_databits_8        CS8

#define rs232_parity_N          0
#define rs232_parity_E          PARENB
#define rs232_parity_O          PARODD

#define rs232_stopbits_1        0
#define rs232_stopbits_2        CSTOPB

#define rs232_flowctrl_IXON     IXON
#define rs232_flowctrl_IXOFF    IXOFF

#define rs232_flowctrl          CRTSCTS

int     rs232_recv( int handle , char *data , int size );

int     rs232_open( char *devName , int nBaudrate );
int     rs232_close(int handle);
int     rs232_read(int handle, char *buffer, long nLen);
int     rs232_write(int handle, char *buffer, long nLen);
int		rs232_WaitingByte(int handle);
int		rs232_SetBaudrate(int handle, int nBaudrate);

void    ptz_arrow_control(int nPort, unsigned char nAddr, unsigned char nArrow);
void    ptz_button_control(int nPort, unsigned char nAddr, unsigned char nButton);
void    ptz_null_control(int nPort, unsigned char nAddr);

//void	Modify_Rs232_Debug( int debug );
void    print_data( unsigned char *data , int size );

#endif
