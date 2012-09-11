#include <stdio.h>
#include <string.h>
#include <ctype.h>

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

        unsigned char *testedArray=( unsigned char * )data;
#if 0
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
#else
        stringLength += sprintf( &printBuf[stringLength] , "0x%x," , testedArray[index] );
        binaryCount++;
#endif
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
