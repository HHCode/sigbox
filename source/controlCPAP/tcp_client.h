#ifndef _TCP_CLIENT_
#define _TCP_CLIENT_

int TCP_ConnectToServer( char *server , int port );
int TCP_Write( int descriptor , char *buffer , int length );
int TCP_Read( int descriptor , char *data , int size );

#endif


