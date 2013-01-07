#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <common.h>

struct timeval start;
//--------------------------------------------------------
// Function Name : Check_Stream
// Purpose       : 為了方便debug時間上的問題，但注意此function不是thread safe的
// Input         :
// Output        : None
// return value  : None
// Modified Date : 2009/11/2 by HawkHsieh
// Notice                :
//--------------------------------------------------------
void Duty_Start( void )
{
        gettimeofday(&start,0);
}


//--------------------------------------------------------
// Function Name : Duty_End
// Purpose       :
// Input         :
// Output        : None
// return value  : None
// Modified Date : 2009/11/2 by HawkHsieh
// Notice                :
//--------------------------------------------------------
int Duty_End( char *tag )
{
        struct timeval end;
        int Cost;
        gettimeofday(&end,0);
        Cost = ((end.tv_sec - start.tv_sec) * 1000000 + end.tv_usec - start.tv_usec);
        start = end;
        printf_counter("%s:Duty cost %dus\n", tag , Cost);
        return Cost;
}

void printData( char *data , int size , char *prefix , int binary )
{
    char printBuf[2048];

    printf( "%s" , prefix );
    int index;
    int stringLength=0;
    int binaryCount=0;
    bzero( printBuf , sizeof(printBuf) );
    for( index=0 ; index<size ; index++ )
    {

        unsigned char *testedArray=( unsigned char * )data;
        if ( binary == 0 )
        {
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
            if ( binaryCount > 10 )
            {
                strcpy( &printBuf[stringLength] , "..." );
                break;
            }
        }
        else
        {
            stringLength += sprintf( &printBuf[stringLength] , "0x%x," , testedArray[index] );
            binaryCount++;
        }
    }
    printf( "%s\n" , printBuf );
}
