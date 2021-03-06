#ifndef _SOCKET2UART_
#define _SOCKET2UART_

#include <pthread.h>

typedef struct{

    int port;
    int listen_fd;
    int connect_fd;
    pthread_t relay_thread_handle;
    pthread_mutex_t mutexSocket2Uart;
    pthread_cond_t condSocket2Uart;
    pthread_t threadSocket2Uart;
    int execution_permit;
    int connect_serial_number;
    char status_string[128];

}Socket2Uart;

void socket2uartRefreshUART( Socket2Uart *socket_to_uart , int uart_descriptor );
void Init_socket2uart( Socket2Uart *socket_to_uart );
int socket2uart( Socket2Uart *socket_to_uart );
int socket2uart_IsConnect( Socket2Uart *socket_to_uart );

void socket2uart_setExecutePermit( Socket2Uart *socket_to_uart , int execution_permit );
int socket2uart_getExecutePermit( Socket2Uart *socket_to_uart );
void socket2uart_closeForced( Socket2Uart *socket_to_uart );
int socket2uart_RefreshConnectID(Socket2Uart *socket_to_uart , int *connect_serial_number );
void socket2uart_SetStatusString( Socket2Uart *socket_to_uart , char *status_string );
void socket2uart_GetStatusString( Socket2Uart *socket_to_uart , char *status_string , int status_string_len );
#endif
