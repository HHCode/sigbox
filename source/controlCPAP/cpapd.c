#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cpap.h>
#include <rs232.h>
#include <dac.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <ctype.h>
#include <common.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "socket2uart.h"
#include <sys/wait.h>

int debug=0;

//-----------For uart to DA-----------
#define PERIOD_OF_PRESSURE_OUTPUT 1
static pthread_t threadTickGenerator;
static pthread_t threadFIFO2DA[MAX_CHANNEL];
static pthread_mutex_t mutexTick[MAX_CHANNEL];
static pthread_cond_t condTick[MAX_CHANNEL];

//-----------For socket to uart-----------
static pthread_t threadSocket2Uart;
static Socket2Uart socket_to_uart;
#define BUF_SIZE 1024
#define SERVPORT 9527



typedef enum{

    APAP_PRESSURE,
    CPAP_PRESSURE,
    PAITENT_FLOW,
    LEAK,
    RAMP,
    PVA,
    NUM_OF_COMMAND

}PERIODIC_COMMAND;


typedef struct {
    PERIODIC_COMMAND command_number;
    char    name[32];
    char    command_code[16];
    int     command_length;

    char    recv_buffer[1024];
    int     recv_length;
    int     expected_recv_length;        //the received data length ,not the buffer length

    char    output_DA;          //channel number,ascii code
    int     max_value;
    int     value_in_recv_buffer;
}CPAPCommand;

/*-----------output to DA-----------
A Mask Pressure 0 to 30 cm H2O 0 to 1V DC
B Patient Flow -120 to 120 L/min -1 to 1V DC
C Leak 0 to 60 L/min 0 to 1V DC
D Inspiration set pressure 0 to 30 cm H2O 0 to 1V DC
E Expiration set pressure 0 to 30 cm H2O 0 to 1V DC
F Minute Ventilation 0 to 30 L 0 to 1V DC
*/
static CPAPCommand command_list[NUM_OF_COMMAND]=
{
    {
        command_number:0,
        name:"APAP Pressure",           //A Mask Pressure 0 to 30 cm H2O 0 to 1V DC
        command_code:{0x93 , 0xd2},
        command_length:2,
        recv_length:5,                  //include xor
        output_DA:'0',
        max_value:30

    },
    {
        command_number:1,
        name:"CPAP Pressure",           //A Mask Pressure 0 to 30 cm H2O 0 to 1V DC
        command_code:{0x93 , 0xcb},
        command_length:2,
        recv_length:7,
        output_DA:'0',
        max_value:30
    },
    {
        command_number:2,
        name:"Patient Flow",            //B Patient Flow -120 to 120 L/min -1 to 1V DC
        command_code:{0x93 , 0xd2},
        command_length:2,
        recv_length:5,
        output_DA:'1',
        max_value:240
    },
    {
        command_number:3,
        name:"Leak",                    //C Leak 0 to 60 L/min 0 to 1V DC
        command_code:{0x93 , 0xcf},
        command_length:2,
        recv_length:7,
        output_DA:'2',
        max_value:60
    },
    {
        command_number:4,
        name:"Ramp Time",
        command_code:{0x93 , 0xd5},
        command_length:2,
        recv_length:7
    },
    {
        command_number:5,
        name:"PVA",
        command_code:{0x93 , 0xd6},
        command_length:5,
        recv_length:1
    }


};

int readAChar( int fifoFD )
{
    char aChar;
    int err = read( fifoFD , &aChar , 1);
    if ( err < 0 )
    {
        perror("read");
        return -1;
    }
    else if ( err == 0 )
    {
        sleep(1);
        return -2;
    }
    return (int)aChar;
}

#define SAMPLING_DURATION_MS 40

void waitTick( int Channel )
{
    pthread_mutex_lock( &mutexTick[Channel] );
    pthread_cond_wait( &condTick[Channel] , &mutexTick[Channel] );
    pthread_mutex_unlock( &mutexTick[Channel] );
}

void *functionTickGenerator( void *param )
{
    int Channel;
    for( Channel=0 ;Channel<MAX_CHANNEL;Channel++)
    {
        pthread_mutex_init( &mutexTick[Channel] , 0 );
        pthread_cond_init( &condTick[Channel] , 0 );
    }
    while(1)
    {
        //double lastTick;
        //double presentTick;
        struct timeval tv;
        struct timezone tz;
        gettimeofday( &tv , &tz );

        //presentTick=tv.tv_sec+tv.tv_usec*0.000001;
        //printf_debug( "tick=%f\n" , presentTick-lastTick );
        //lastTick=presentTick;

        for( Channel=0 ;Channel<MAX_CHANNEL;Channel++)
        {
            pthread_mutex_lock( &mutexTick[Channel] );
            pthread_cond_signal( &condTick[Channel] );
            pthread_mutex_unlock( &mutexTick[Channel] );
        }

        struct timespec remainTime;
        bzero( &remainTime , sizeof(struct timespec) );
        while(1)
        {
           struct timespec sleepTime;
           sleepTime.tv_nsec=SAMPLING_DURATION_MS*1000000;
           sleepTime.tv_sec=0;
           if ( nanosleep(&sleepTime , &remainTime ) == 0 )
               break;
        }
    }


}
#define ALPHA_NUM_OF_16BIT 4


int OpenFIFO( char *fifo_path )
{
    int fifoFD;
    fifoFD = open( fifo_path , O_RDONLY );
    if ( fifoFD < 0 )
    {
        printf_error("open %s error , try to create\n" , fifo_path );
        perror("open");
        if ( mkfifo( fifo_path , 0777 ) )
        {
            printf_error("create %s error , exit\n" , fifo_path );
            perror("mkfifo");
            return -1;
        }
    }
    return fifoFD;
}


int ReadFIFO( int descriptor , char *read_buffer , int buffer_size )
{
    int readSize;

    do
    {
        readSize = read( descriptor , read_buffer ,  buffer_size );
        struct timespec remainTime;
        struct timespec sleepTime;

        if ( readSize < 0 )
        {
            printf_error( "read error, fd=%d\n" , descriptor );
            perror( "read" );
        }
        sleepTime.tv_nsec=SAMPLING_DURATION_MS*100000;
        sleepTime.tv_sec=0;
        nanosleep( &sleepTime , &remainTime );

    }while( readSize <=0 );

    return readSize;
}

void *functionFIFO2DA( void *param )
{
    char    fifo_path[32];
    int     fifoFD;
    int     channel=(int)param;
    pthread_detach( pthread_self() );
    sprintf( fifo_path , "/tmp/FIFO_CHANNEL%d" , channel );
    fifoFD = OpenFIFO( fifo_path );
    if ( fifoFD < 0 )
        exit(1);

    while(1)
    {
        char readBuffer[512*1024];
        char *walkInBuffer=readBuffer;
        int stringIndex;
        int readInt;
        char *toNullPos;
        int readSize;


        readSize = ReadFIFO( fifoFD , readBuffer , sizeof(readBuffer));

        if ( readSize <=0 )
        {
            printf_error("ReadFIFO error\n");
            sleep(1);
            continue;
        }
        if ( debug ) printf( "ch%d:" , channel );
        do
        {
            toNullPos = strchr( walkInBuffer , ',');
            if ( toNullPos )
                *toNullPos=0;
            stringIndex=sscanf( walkInBuffer , "0x%x" , &readInt );
            if ( stringIndex == 0 )
            {
                stringIndex=sscanf( walkInBuffer , "%d" , &readInt );
                if ( stringIndex == 0 )
                {
                    printf_error("an invalid number %s\n" , walkInBuffer );
                    if ( toNullPos )
                    {
                        walkInBuffer=toNullPos+1;
                        continue;
                    }
                    else
                        break;
                }
            }
            writeDAC( channel , readInt );
            waitTick( channel );
            if ( toNullPos )
                walkInBuffer=toNullPos+1;
            else
                break;

        }while( strlen(walkInBuffer) );
        if ( debug ) printf( "\n" );
    }

    printf( "Channel %d exit\n" , channel );

    return 0;
}



int GetDAValue( PERIODIC_COMMAND command_number , int max_value , char *recv_buffer )
{
    int adjuested_value;

    switch( command_number )
    {
    case APAP_PRESSURE:
        adjuested_value = (65535.0 / max_value )*recv_buffer[2];
        break;

    case CPAP_PRESSURE:
        adjuested_value = (65535.0 / max_value )*recv_buffer[2];
        break;

    case PAITENT_FLOW:
    {
        char low=recv_buffer[2];
        char hight=recv_buffer[3];
        adjuested_value = (65535.0 / max_value )*( hight << 8 | low ) ;
        break;
    }
    case LEAK:
    {
        char low=recv_buffer[2];
        char hight=recv_buffer[3];
        adjuested_value = (65535.0 / max_value )*( hight << 8 | low ) ;
        break;
    }
    default:
        break;
    }


    return adjuested_value;
}


int CPAPSendCommand( int deviceDesc , CPAPCommand *command )
{
    char checkedXor;

    checkedXor = getCheckedXor( command->command_code , command->command_length );

    if ( sendCPAPCmd( deviceDesc , command->command_code , command->command_length , checkedXor ) )
        return -1;

    int responseSize;
    responseSize = recvCPAPResponse( deviceDesc , command->recv_buffer , sizeof( command->recv_buffer ) , command->expected_recv_length );

    return responseSize;
}

int cpap2psg( int rs232_descriptor , CPAPCommand *command )
{
    int ret=0;

    command->recv_length = CPAPSendCommand( rs232_descriptor , command );

    if ( command->recv_length < 0  )
    {
        ret= command->recv_length;
        goto EndOf_cpap2psg;
    }

    if ( debug )
        printData( command->recv_buffer , command->recv_length , "uart >>>");

    if ( command->output_DA )
    {
        int adjustedValue;
        adjustedValue = GetDAValue( command->command_number , command->max_value , command->recv_buffer );
        printf_debug( "%s >> DA: %d\n" , command->name , adjustedValue );
    }

EndOf_cpap2psg:
    return ret;
}

int GetCommandFromFIFO( int fifoFD , char *command_code , int buffer_size , int *hex_cmd_size , int *expected_length )
{
    int read_size=0;
    int index=0;
    char read_buffer[64];

    *hex_cmd_size=0;
    read_size = ReadFIFO( fifoFD , read_buffer , buffer_size );

    if ( read_size <= 0 )
    {
        printf_error( "ReadFIFO error:%d\n" , read_size );
        return -1;
    }

    index = sscanf( read_buffer , "expected=%d,cmd_size=%d,hex=" , expected_length , hex_cmd_size);

    if ( index > 0 && *hex_cmd_size > 0 )
    {
        char *hex_cmd;
        hex_cmd=strstr( read_buffer,"hex=" );

        if ( hex_cmd )
            hex_cmd += strlen("hex=");
        else
        {
            printf_error( "leak of hex=\n");
            return -1;
        }
        printf_debug("index=%d,expected=%d,cmd_size=%d\n" , index , *expected_length , *hex_cmd_size );
        int hex_cmd_index=0;

        for( hex_cmd_index=0 ; hex_cmd_index < *hex_cmd_size ; hex_cmd_index++ )
        {
            command_code[hex_cmd_index] = hex_cmd[hex_cmd_index];
            printf_debug( "[%d],0x%x\n" , hex_cmd_index , command_code[hex_cmd_index] );
        }
    }
    else
    {
        printf_error( "sscanf error,index=%d\n" , index );
        return -1;
    }

    return read_size;
}



void *functionSocket2Uart( void *param )
{
    Socket2Uart *socket_to_uart=(Socket2Uart *)param;

    socket2uart( socket_to_uart );
    return 0;
}




int ExecuteSeriesCommand( int *rs232_descriptor )
{
    int command_index;
    int is_CPAP_mode=0;
    for( command_index=0 ; command_index<NUM_OF_COMMAND ; command_index++ )
    {
        //choose one of mode
        if ( is_CPAP_mode && command_index == APAP_PRESSURE )
            continue;

        if ( is_CPAP_mode==0 && command_index == CPAP_PRESSURE )
            continue;

        int err;
        err=cpap2psg( *rs232_descriptor , &command_list[command_index] );
        if ( err == -2 )
        {
            close( *rs232_descriptor );
            *rs232_descriptor = openCPAPDevice();
        }
    }

    return 0;
}

int cpapd( void )
{
    int rs232_descriptor;
    initDAC();

    int deviceDesc;
    deviceDesc = openCPAPDevice();

    socket_to_uart.uart_fd=deviceDesc;

    pthread_create( &threadTickGenerator , 0 , functionTickGenerator , 0 );

    pthread_create( &threadSocket2Uart , 0 , functionSocket2Uart , &socket_to_uart );

    while(1)
    {
        if ( socket2uart_IsConnect( &socket_to_uart ) == 0 )
            ExecuteSeriesCommand( &deviceDesc );

        int channel;
        for( channel=0 ; channel<MAX_CHANNEL ; channel++ )
        {
            if ( threadFIFO2DA[channel] == 0 )
                pthread_create( &threadFIFO2DA[channel] ,0 , functionFIFO2DA , (void *)channel );
        }

        sleep( PERIOD_OF_PRESSURE_OUTPUT );
    }
    rs232_close( rs232_descriptor );
    return 0;
}

void handler(int sig)
{
    pid_t pid;

    pid = wait(NULL);

    printf("Pid %d exited.\n", pid);

    if (!fork())
    {
        printf("Child pid is %d\n", getpid());
        cpapd();
    }
}


int main( int argc , char **argv )
{
    if ( access( "/etc/debug" , 0 ) == 0 )
        debug=1;

    socket_to_uart.port=SERVPORT;

    if ( argc == 3)
    {
        socket_to_uart.port = atoi( argv[1] );
        strncpy( socket_to_uart.uart_name , argv[2] , sizeof( socket_to_uart.uart_name) );
    }
    else
    {
        printf("example:cpapd 21 /dev/ttyUSB0\n");
        exit(1);
    }
    printf_debug("listen port:%d\n" , socket_to_uart.port);
    /* Variable and structure definitions. */

#ifdef UNDEAD
    signal(SIGCHLD, handler);
    if (!fork())
    {
        return cpapd();
    }
#else
        cpapd();
#endif

    while(1) sleep(10); //father process

    return 0;
}
