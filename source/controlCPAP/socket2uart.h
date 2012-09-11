#ifndef _SOCKET2UART_
#define _SOCKET2UART_

#include <pthread.h>

typedef struct{

    int uart_fd;
    int port;
    char uart_name[32];
    int listen_fd;
    int connect_fd;
    pthread_t relay_thread_handle;
    pthread_mutex_t mutexSocket2Uart;
    pthread_cond_t condSocket2Uart;
}Socket2Uart;


int socket2uart( Socket2Uart *socket_to_uart );
int socket2uart_IsConnect( Socket2Uart *socket_to_uart );

#endif
