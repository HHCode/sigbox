#ifndef _DAC_H
#define _DAC_H
#include <stdint.h>
#define MAX_CHANNEL 6

int initDAC( void );
int writeDAC( int channel , uint16_t adjustedValue );

#endif
