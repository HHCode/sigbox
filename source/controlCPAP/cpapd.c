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
#include <signal.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>

#ifndef WDIOC_SETTIMEOUT
 #define WDIOC_SETTIMEOUT        _IOWR(WATCHDOG_IOCTL_BASE, 6, int)
 #define WDIOC_GETTIMEOUT        _IOR(WATCHDOG_IOCTL_BASE, 7, int)
 #define WDIOF_SETTIMEOUT        0x0080
#endif


int debug=0;

//-----------For uart to DA-----------
#define PERIOD_OF_PRESSURE_OUTPUT 1
static pthread_t threadTickGenerator;
static pthread_t threadFIFO2DA[MAX_CHANNEL];
static pthread_mutex_t mutexTick[MAX_CHANNEL];
static pthread_cond_t condTick[MAX_CHANNEL];

//-----------For socket to uart-----------
static Socket2Uart socket_to_uart;
#define BUF_SIZE 1024
#define SERVPORT 9527

#define  PERIOD_COMMAND_RETRY 20


typedef enum{

    MODE,
    CPAP_PRESSURE,
    APAP_PRESSURE,
    PATIENT_FLOW,
    LEAK,
    RAMP,
    PVA,
    BLOW,
    NUM_OF_COMMAND,


    SET_CPAP_PRESSURE,
    SET_APAP_PRESSURE


}PERIODIC_COMMAND;


typedef struct {
    PERIODIC_COMMAND command_number;
    char    name[32];
    char    command_code[16];
    int     command_length;

    uint8_t recv_buffer[1024];
    int     recv_length;
    int     expected_recv_length;        //the received data length ,not the buffer length

    char    output_DA;          //channel number,ascii code
    int     max_value;
    int     value_in_recv_buffer;
    int     dec_count;
    int     sample_count;
}CPAPCommand;


static CPAPCommand set_cpap_pressure=
{
    command_number:SET_CPAP_PRESSURE,
    name:"CPAP_PRESSURE",
    command_code:{0x93 , 0xcd},
    command_length:3,
    expected_recv_length:1                  //include xor
};

static CPAPCommand set_apap_pressure=
{
    command_number:SET_APAP_PRESSURE,
    name:"APAP_PRESSURE",
    command_code:{0x93 , 0xd1},
    command_length:5,
    expected_recv_length:1                  //include xor
};

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
        command_number:MODE,
        name:"Mode",           //A Mask Pressure 0 to 30 cm H2O 0 to 1V DC
        command_code:{0x93 , 0xc4},
        command_length:2,
        expected_recv_length:4,                  //include xor
        sample_count:10
    },
    {
        command_number:CPAP_PRESSURE,
        name:"CPAP Pressure",           //A Mask Pressure 0 to 30 cm H2O 0 to 1V DC
        command_code:{0x93 , 0xcb},
        command_length:2,
        expected_recv_length:5,                  //include xor
        output_DA:'0',
        max_value:200,
        sample_count:10
    },
    {
        command_number:APAP_PRESSURE,
        name:"APAP Pressure",           //A Mask Pressure 0 to 30 cm H2O 0 to 1V DC
        command_code:{0x93 , 0xd2},
        command_length:2,
        expected_recv_length:8,
        output_DA:'0',
        max_value:200,
        sample_count:1
    },
    {
        command_number:PATIENT_FLOW,
        name:"Patient Flow",            //B Patient Flow -120 to 120 L/min -1 to 1V DC
        command_code:{0x93 , 0xf1},
        command_length:2,
        expected_recv_length:5,
        output_DA:'1',
        max_value:2000,
        sample_count:0
    },
    {
        command_number:LEAK,
        name:"Leak",                    //C Leak 0 to 60 L/min 0 to 1V DC
        command_code:{0x93 , 0xcf},
        command_length:2,
        expected_recv_length:5,
        output_DA:'2',
        max_value:200,
        sample_count:0
    },
    {
        command_number:RAMP,
        name:"Ramp Time",
        command_code:{0x93 , 0xd5},
        command_length:2,
        expected_recv_length:5,
        sample_count:50
    },
    {
        command_number:PVA,
        name:"PVA",
        command_code:{0x93 , 0xd6},
        command_length:2,
        expected_recv_length:5,
        sample_count:50
    },
    {
        command_number:BLOW,
        name:"BLOW",
        command_code:{0x93 , 0x52},
        command_length:2,
        expected_recv_length:4,
        sample_count:50
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
    int     retry=3;
    pthread_detach( pthread_self() );
    sprintf( fifo_path , "/tmp/FIFO_CHANNEL%d" , channel );

    do{
        fifoFD = OpenFIFO( fifo_path );
    }while( fifoFD < 0 && retry-- > 0 );

    if ( retry < 0)
    {
        printf_error("open %s error\n" , fifo_path );
        exit(1);
    }

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
        if ( recv_buffer[5] > max_value ) recv_buffer[5] = max_value;
        adjuested_value = (65535.0 / max_value )*recv_buffer[5];
        break;

    case CPAP_PRESSURE:
        if ( recv_buffer[2] > max_value ) recv_buffer[2] = max_value;
        adjuested_value = (65535.0 / max_value )*recv_buffer[2];
        break;

    case PATIENT_FLOW:
    {
        char low=recv_buffer[2];
        char hight=recv_buffer[3];
        uint16_t flow_integer=( hight << 8 | low );
     //   printf("0x%x%x , %d\n" , hight , low , flow_integer );
        if ( flow_integer > max_value ) flow_integer = max_value;
        adjuested_value = (65535.0 / max_value )*( flow_integer ) ;
        break;
    }
    case LEAK:
    {
        char low=recv_buffer[2];
        char hight=recv_buffer[3];
        uint16_t leak_integer=( hight << 8 | low );
        if ( leak_integer > max_value ) leak_integer = max_value;
        adjuested_value = (65535.0 / max_value )*( leak_integer ) ;
        break;
    }
    default:
        break;
    }


    return adjuested_value;
}


int CPAPSendCommand( CPAPCommand *command )
{
    printf_debug("send %s command\n" , command->name );
    return CPAP_SendCommand( command->command_code , command->command_length , command->recv_buffer , sizeof(command->recv_buffer) , command->expected_recv_length );
}

int cpap2psg( CPAPCommand *command )
{
    int ret=-1;
    int retry;
    for( retry=0 ; retry< PERIOD_COMMAND_RETRY ; retry++ )
    {
        command->recv_length = CPAPSendCommand( command );

        if ( command->recv_length < 0  )
        {
            if ( debug )
            {
                printf_debug("CPAPSendCommand error\n");
                printData( (char *)command->recv_buffer , command->recv_length , "send data error\n" , 1 );
            }
            continue;
        }


        if ( command->recv_length == 1 && isErrorCode( command->recv_buffer[0] ) )
        {
            printf_error( "cmd %s has problem\n" , command->name );
            break;
        }

        ret=0;

        if ( debug && command->recv_length > 0 )
            printData( (char *)command->recv_buffer , command->recv_length , "uart >>>" , 1 );

        if ( command->output_DA )
        {
            int adjustedValue;
            adjustedValue = GetDAValue( command->command_number , command->max_value , (char *)command->recv_buffer );
            printf_debug( "%s:%c >> DA: 0x%x\n" , command->name , command->output_DA , adjustedValue );
            writeDAC( command->output_DA-'0' , adjustedValue );
        }

        break;
    }

    if ( retry >= PERIOD_COMMAND_RETRY )
        printf_debug( "cmd %s retry over %d times give up\n" , command->name , PERIOD_COMMAND_RETRY );

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


int IsCPAP( void )
{
    if ( command_list[MODE].recv_buffer[2] == 0 )
        return 1;
    else
        return 0;
}

int GetTherapyPressure( void )
{
    return command_list[CPAP_PRESSURE].recv_buffer[2];
}


int GetRampTime( void )
{
    return command_list[RAMP].recv_buffer[2];
}

int GetRampStartPressure( void )
{
    return command_list[RAMP].recv_buffer[3];
}

int GetInitPressure( void )
{
    return command_list[APAP_PRESSURE].recv_buffer[2];
}

int GetMaxPressure( void )
{
    return command_list[APAP_PRESSURE].recv_buffer[3];
}

int GetMinPressure( void )
{
    return command_list[APAP_PRESSURE].recv_buffer[4];
}


int GetPVA( void )
{
    return command_list[PVA].recv_buffer[2];
}

int GetPVALevel( void )
{
    return command_list[PVA].recv_buffer[3];
}


int GetPatientFlow( void )
{
    int flow=command_list[PATIENT_FLOW].recv_buffer[3]<<8 | command_list[PATIENT_FLOW].recv_buffer[2];
    return flow;
}


int GetLeak( void )
{
    int leak_value=command_list[LEAK].recv_buffer[3]<<8 | command_list[LEAK].recv_buffer[2];
    return leak_value;
}

int GetPressure( void )
{
    int present_pressure;
    if ( IsCPAP() )
    {
        present_pressure = command_list[CPAP_PRESSURE].recv_buffer[2];
    }
    else
    {
        present_pressure = command_list[APAP_PRESSURE].recv_buffer[5];
    }
    return present_pressure;
}



int GetCPAPPressure( void )
{
    int present_pressure=command_list[CPAP_PRESSURE].recv_buffer[2];
    return present_pressure;
}


int GetAPAPPressure( void )
{
    int present_pressure= command_list[APAP_PRESSURE].recv_buffer[5];
    return present_pressure;
}


int GetBlow( void )
{
    return (command_list[BLOW].recv_buffer[2] == 0xe5)?0:1;
}


int CPAPSendCommandDebug( CPAPCommand *command )
{
    command->recv_length = CPAPSendCommand( command );

    if ( command->recv_length < 0  )
    {
        if ( debug )
        {
            printf_debug("CPAPSendCommand error\n");
            printData( (char *)command->recv_buffer , command->recv_length , "send data error\n" , 1 );
        }
    }
    return command->recv_length;
}

int ExecuteSeriesCommand( void )
{
    int command_index;
    static int is_CPAP_mode=0;
    int err=0;


    usleep(40000);

    Duty_Start();

    if ( command_list[MODE].dec_count > 0  )
    {
        command_list[MODE].dec_count--;
    }
    else
    {
       command_list[MODE].dec_count = command_list[MODE].sample_count;
       if ( CPAPSendCommand( &command_list[MODE] ) > 0  )
       {
           Duty_End("MODE");

           int pressure;
           if ( IsCPAP() )
           {
               pressure = GetAPAPPressure();
               if ( pressure == 0 )
                   pressure = GetCPAPPressure();
               printf_debug("detect CPAP mode\n");
               if ( is_CPAP_mode == 0 )
               {
                   printf_info("detect APAP->CPAP mode\n");
                   int set_pressure=(double)pressure*0.2;
                   printf_info("set CPAP pressure=%d\n" , set_pressure );
                   set_cpap_pressure.command_code[2]= set_pressure;
                   CPAPSendCommandDebug( &set_cpap_pressure );
               }
               is_CPAP_mode = 1;
           }
           else
           {
               pressure = GetCPAPPressure();
               if ( pressure == 0 )
                   pressure = GetAPAPPressure();

               printf_debug("detect APAP mode\n");
               if ( is_CPAP_mode == 1 )
               {
                   int set_pressure=(double)pressure*0.2;
                   printf_info("detect CPAP->APAP mode\n");
                   if ( pressure > GetMaxPressure()*5 )
                   {
                       printf_info("over max %d\n" , GetMaxPressure()*5 );
                       set_pressure = GetMaxPressure()-1;
                   }
#if 0
                   if ( pressure < GetMinPressure()*5 )
                   {
                       printf_info("below min %d\n" , GetMinPressure()*5 );
                       set_pressure = GetMinPressure();
                   }
#endif
                   printf_info("set APAP pressure=%d\n" , set_pressure );
                   set_apap_pressure.command_code[2]=GetInitPressure();
                   set_apap_pressure.command_code[3]=GetMaxPressure();
                   set_apap_pressure.command_code[4]=set_pressure;
                   CPAPSendCommandDebug( &set_apap_pressure );
               }
               is_CPAP_mode=0;
           }
       }
    }

        for( command_index=CPAP_PRESSURE ; command_index<NUM_OF_COMMAND ; command_index++ )
        {
            if ( command_list[command_index].dec_count > 0  )
            {
                command_list[command_index].dec_count--;
                continue;
            }
            else
                command_list[command_index].dec_count = command_list[command_index].sample_count;

            //choose one of mode
            if ( is_CPAP_mode && command_index == APAP_PRESSURE )
                continue;

            if ( is_CPAP_mode==0 && command_index == CPAP_PRESSURE )
                continue;

            err=cpap2psg( &command_list[command_index] );
            Duty_End( command_list[command_index].name );


            static struct timeval last_time;
            struct timeval present_time;

            gettimeofday( &present_time , 0 );

            int diff_usec=present_time.tv_sec*1000000+present_time.tv_usec - last_time.tv_sec*1000000+last_time.tv_usec;
            printf_debug( "D/A period is %d ms , %f Hz\n" ,  diff_usec/1000 , 1000000.0/diff_usec );

            last_time = present_time;
        }

    char status_command[256];
    static uint32_t serial_number;
    if ( IsCPAP() )
    {
        //Mode=CPAP\n,TherapyPressure=%d\nRampTime=%d\nRampStartPressure=%d\nPVA=%d\nPVALevel=%d\n
        snprintf( status_command , sizeof(status_command),"CPAP,%d,%d,%d,%s,%d,%d,%d,%d,%s,%d" ,
                  GetTherapyPressure(),
                  GetRampTime(),
                  GetRampStartPressure(),
                  GetPVA()?"ON":"OFF",
                  GetPVALevel(),
                  GetPressure(),
                  GetLeak(),
                  GetPatientFlow(),
                  GetBlow()?"ON":"OFF",
                  serial_number++
                 );
    }
    else
    {
        //"Mode=APAP\n,MaxPressure=%d\nMinPressure=%d\nInitPressure=%d\nPVA=%d\nPVALevel=%d\n"
        snprintf( status_command , sizeof(status_command) , "APAP,%d,%d,%d,%s,%d,%d,%d,%d,%s,%d" ,
                  GetMaxPressure(),
                  GetMinPressure(),
                  GetInitPressure(),
                  GetPVA()?"ON":"OFF",
                  GetPVALevel(),
                  GetPressure(),
                  GetLeak(),
                  GetPatientFlow(),
                  GetBlow()?"ON":"OFF",
                  serial_number++
                 );

    }
    socket2uart_SetStatusString( &socket_to_uart , status_command );
    Duty_End( "EndOfCommand" );
    return err;
}

int is_socket2uart_timeout( struct timeval *connect_time )
{
    struct timeval present_time;
    gettimeofday( &present_time , 0 );
    int timeout_second = (present_time.tv_sec - connect_time->tv_sec);
    int timeout_usecond = (present_time.tv_usec - connect_time->tv_usec );

    if ( timeout_second > 0 )
        return timeout_second*1000000;
    else
    {
        if ( timeout_usecond > 500000 )
            return timeout_usecond;
    }

    return 0;
}

static void sigpipe_handler(int sig)
{

}


int initWatchDog( int second )
{
    int watchdog_fd;
    watchdog_fd = open("/dev/watchdog", O_RDWR);
    if (-1 == watchdog_fd)
    {
        perror("open");
        return -1;
    }

    if ( ioctl( watchdog_fd , WDIOC_SETTIMEOUT, &second ))
    {
        perror("ioctl");
        return -1;
    }

    int present_second;
    if ( ioctl( watchdog_fd , WDIOC_GETTIMEOUT, &present_second ))
    {
        perror("ioctl");
        return -1;
    }

    printf( "watch dog %d -> %d second\n"  , present_second ,  second );
    return watchdog_fd;
}

void ResetWatchDog( int watchdog_fd )
{
    int dummy=0;
    if ( ioctl( watchdog_fd , WDIOC_KEEPALIVE , 0 ) )
    {
        perror("ioctl");
    }

    write( watchdog_fd , &dummy , sizeof(dummy) );
}

int cpapd( void )
{
    struct timeval connect_time;
    int socket2uart_permit=0;
    int serial_number=0;
    int last_serial_number;
    signal(SIGPIPE, sigpipe_handler);

//    int watchdog;
 //   watchdog = initWatchDog( 5 );
    initDAC();
    Init_CPAP();
    pthread_create( &threadTickGenerator , 0 , functionTickGenerator , 0 );
    Init_socket2uart( &socket_to_uart );
    while(1)
    {
   //     ResetWatchDog( watchdog );
        if ( socket2uart_IsConnect( &socket_to_uart ) == 0 )
        {
            last_serial_number = serial_number;
            memset( &connect_time , 0 ,sizeof(connect_time));
            ExecuteSeriesCommand();
      //      sleep(1);
        }
        else if ( connect_time.tv_sec == 0 )
        {
            gettimeofday( &connect_time , 0 );
            socket2uart_setExecutePermit( &socket_to_uart , 1 );
            socket2uart_permit = 1;
        }
        else
        {
            if ( is_socket2uart_timeout( &connect_time ) > 1000000 )
            {
                printf_error("close client since 1s time out\n");
                socket2uart_closeForced( &socket_to_uart );
            }
            else if ( is_socket2uart_timeout( &connect_time ) > 500000 && socket2uart_permit == 1 )
            {
                socket2uart_permit = 0;
                printf_debug("set permit invalid\n");
                socket2uart_setExecutePermit( &socket_to_uart , 0 );
            }
            else if ( socket2uart_RefreshConnectID( &socket_to_uart , &serial_number ) )
                memset( &connect_time , 0 ,sizeof(connect_time));
        }

        int channel;
        for( channel=0 ; channel<MAX_CHANNEL ; channel++ )
        {
            if ( threadFIFO2DA[channel] == 0 )
                pthread_create( &threadFIFO2DA[channel] ,0 , functionFIFO2DA , (void *)channel );
        }
    }
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

    if ( argc == 2)
    {
        socket_to_uart.port = atoi( argv[1] );
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


