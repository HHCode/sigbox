#ifndef _COMMON_H_
#define _COMMON_H_

extern int debug;


#define printf_debug(fmt, args...)\
{\
    if ( debug ) printf("%s[%d]: "fmt, __FILE__, __LINE__, ##args);\
}


#define printf_error(fmt, args...)\
{\
    fprintf(stderr, "FAILED\n" ); \
    fprintf(stderr, "%s[%d]: "fmt, __FILE__, __LINE__, ##args); \
}

void printData( char *data , int size , char *prefix );

#endif
