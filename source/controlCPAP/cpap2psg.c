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
static pthread_t threadTickGenerator;
static pthread_t threadFIFO2DA[MAX_CHANNEL];
static pthread_mutex_t mutexTick[MAX_CHANNEL];
static pthread_cond_t condTick[MAX_CHANNEL];


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
        double lastTick;
        double presentTick;
        struct timeval tv;
        struct timezone tz;
        gettimeofday( &tv , &tz );

        presentTick=tv.tv_sec+tv.tv_usec*0.000001;
        //printf_debug( "tick=%f\n" , presentTick-lastTick );
        lastTick=presentTick;

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

void *functionFIFO2DA( void *param )
{
    char    stringFIFO[32];
    int     fifoFD;
    int     channel=(int)param;
    pthread_detach( pthread_self() );
    sprintf( stringFIFO , "/tmp/FIFO_CHANNEL%d" , channel );
    fifoFD = open( stringFIFO , O_RDONLY );
    if ( fifoFD < 0 )
    {
        printf_error("open %s error , try to create\n" , stringFIFO );
        perror("open");
        if ( mkfifo( stringFIFO , 0777 ) )
        {
            printf_error("create %s error , exit\n" , stringFIFO );
            perror("mkfifo");
            goto threadError;
        }
    }

    while(1)
    {
        char readBuffer[512*1024];
        char *walkInBuffer=readBuffer;
        int readSize;
        int stringIndex;
        int readInt;
        char *toNullPos;
        readSize = read( fifoFD , readBuffer , sizeof(readBuffer));

        if ( readSize <=0 )
        {
            struct timespec remainTime;
            struct timespec sleepTime;

            sleepTime.tv_nsec=SAMPLING_DURATION_MS*100000;
            sleepTime.tv_sec=0;
            nanosleep( &sleepTime , &remainTime );
            continue;
        }

        printf_debug( "ch%d:" , channel );
        do
        {
            toNullPos = strchr( walkInBuffer , ',');
            if ( toNullPos )
                *toNullPos=0;
            printf_debug( "get %s\n", walkInBuffer );
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
            printf_debug( "0x%x," , readInt );
            writeDAC( 0 , readInt );
            waitTick( channel );
            if ( toNullPos )
                walkInBuffer=toNullPos+1;
            else
                break;

        }while( strlen(walkInBuffer) );
        printf_debug( "\n" );
    }

threadError:
    printf( "Channel %d exit\n" , channel );

    return 0;
}


int cpap2psg( int rs232_descriptor , char *cmdBuffer , int cmdSize , char checkedXor )
{
    int expectedLength=5;

    if ( sendCPAPCmd( rs232_descriptor , cmdBuffer , cmdSize , checkedXor ) )
        exit( EXIT_FAILURE );

    unsigned char responseBuffer[1024];
    int responseSize;
    responseSize = recvCPAPResponse( rs232_descriptor , responseBuffer , sizeof(responseBuffer) , expectedLength );

    if ( responseSize == -1 )
    {
        usleep(500000);
        return -1;
    }
//        print_data( responseBuffer , responseSize );
    printf("%d\n",responseBuffer[2] );
    writeDAC( 0 , responseBuffer[2] );
    usleep(200000);
    return 0;
}





int main( int argc , char **argv )
{

    int rs232_descriptor;
    char cmdBuffer[2]={ 0x93,0xCB };

    int cmdSize=sizeof(cmdBuffer);
    int checkedXor;

    initDAC();

    int deviceDesc;
    deviceDesc = openCPAPDevice();

    checkedXor = getCheckedXor( cmdBuffer , cmdSize );

    pthread_create( &threadTickGenerator , 0 , functionTickGenerator , 0 );
    while(1)
    {
        //cpap2psg( deviceDesc , cmdBuffer , cmdSize , checkedXor );
        int channel;
        for( channel=0 ; channel<MAX_CHANNEL ; channel++ )
        {
            if ( threadFIFO2DA[channel] == 0 )
                pthread_create( &threadFIFO2DA[channel] ,0 , functionFIFO2DA , (void *)channel );
        }
        sleep(1);
    }

    rs232_close( rs232_descriptor );


    return 0;
}
