#ifndef _COMMON_H_
#define _COMMON_H_

#include <errno.h>
extern int debug;
extern int counter;
extern int rt_debug;

#define printf_info(fmt, args...)\
{\
    printf("%s[%d]: "fmt, __FILE__, __LINE__, ##args);\
}

#define printf_rt_debug(fmt, args...)\
{\
    if ( rt_debug ) printf("%s[%d]: "fmt, __FILE__, __LINE__, ##args);\
}


#define printf_debug(fmt, args...)\
{\
    if ( debug ) printf("%s[%d]: "fmt, __FILE__, __LINE__, ##args);\
}


#define printf_counter(fmt, args...)\
{\
    if ( counter ) printf("%s[%d]: "fmt, __FILE__, __LINE__, ##args);\
}

#define printf_error(fmt, args...)\
{\
    fprintf(stderr, "FAILED\n" ); \
    fprintf(stderr, "%s[%d]: "fmt, __FILE__, __LINE__, ##args); \
}


#define printf_errno(fmt, args...) \
    do {\
    int backuped_errno=errno;\
    char errstr[64];\
    strerror_r( backuped_errno , errstr, sizeof(errstr));\
    fprintf(stderr, "FAILED\n" ); \
    fprintf(stderr, " ERROR  (%s|%s|%d): errno=%d ,%s : " fmt , __FILE__, __func__, __LINE__ , backuped_errno , errstr , ##args );\
    } while(0)

void printData( char *data , int size , char *prefix , int binary );

int Duty_End( char *tag );
void Duty_Start( void );

#endif
