#include "dac.h"
#include "math.h"
#include "stdio.h"

#define ONE_PERIOD_SAMPLE 1500
#define MAX_CHANNEL 6
#define BASE_FREQ 100

int main( int argc , char **argv )
{
    int BaseFrequency=BASE_FREQ;
    int OnePerionSample=ONE_PERIOD_SAMPLE;
    int MaxChannel=6;
    int sample;
    int channel;

    uint16_t signal_buf[MaxChannel][OnePerionSample];
    printf("init dac\n" );
    initDAC();

    printf("create sin wave signal\n" );
    for( channel=0 ; channel<MaxChannel ; channel++ )
    {
        for( sample=0 ; sample<OnePerionSample ; sample++ )
        {
            double sin_sample = sin( 2.0*3.14159265 * BaseFrequency * (double)sample/OnePerionSample);
            signal_buf[channel][sample] = (uint16_t)(32767.0*(sin_sample+1));
        }
    }

    printf("output sin wave" );
    while(1)
    {
        for( sample=0 ; sample<OnePerionSample ; sample++ )
        {
            for( channel=0 ; channel<MaxChannel ; channel++ )
            {
                writeDAC( channel , signal_buf[channel][sample] );
            }
        }
    }

    return 0;
}
