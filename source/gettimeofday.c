#include <sys/time.h>
#include <stdio.h>

void main (int argc, char *argv[])
{
struct timeval tv;
struct timezone tz;
printf( "\n%s:" , argv[1] );
gettimeofday ( &tv , &tz );
printf ( "%d.%06d" , tv.tv_sec , tv.tv_usec);
printf( " %s\n" , argv[2] );

}
