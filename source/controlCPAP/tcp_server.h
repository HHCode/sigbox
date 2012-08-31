#ifndef _TCP_SERVER_
#define _TCP_SERVER_

int read_socket( int connect_fd , char *buffer );
int start_tcp_server( int *listen_fd , int port );

#endif


