/* server.c */

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main()
{
  /* Create the listener socket as TCP socket */
  int sock = socket( PF_INET, SOCK_STREAM, 0 );

  if ( sock < 0 )
  {
    perror( "socket() failed" );
    exit( EXIT_FAILURE );
  }

  /* socket structures */
  struct sockaddr_in server;

  server.sin_family = PF_INET;
  server.sin_addr.s_addr = INADDR_ANY;

  unsigned short port = 8127;

  /* htons() is host-to-network-short for marshalling */
  /* Internet is "big endian"; Intel is "little endian" */
  server.sin_port = htons( port );
  int len = sizeof( server );

  if ( bind( sock, (struct sockaddr *)&server, len ) < 0 )
  {
    perror( "bind() failed" );
    exit( EXIT_FAILURE );
  }

  listen( sock, 5 );   /* 5 is the max number of waiting clients */
  printf( "PARENT: Listener bound to port %d\n", port );

  struct sockaddr_in client;
  int fromlen = sizeof( client );

  int pid;
  char buffer[ BUFFER_SIZE ];

  while ( 1 )
  {
    printf( "PARENT: Blocked on accept()\n" );
    int newsock = accept( sock, (struct sockaddr *)&client,
                          (socklen_t*)&fromlen );
    printf( "PARENT: Accepted client connection\n" );

    /* handle socket in child process */
    pid = fork();

    if ( pid < 0 )
    {
      perror( "fork() failed" );
      exit( EXIT_FAILURE );
    }
    else if ( pid == 0 )
    {
      int n;

#if 0
sleep( 10 );
#endif

do
{
      printf( "CHILD %d: Blocked on recv()\n", getpid() );

      /* can also use read() and write()..... */
      n = recv( newsock, buffer, BUFFER_SIZE, 0 );

      if ( n < 0 )
      {
        perror( "recv() failed" );
      }
else if ( n == 0 )
{
  printf( "CHILD %d: Rcvd 0 from recv(); closing socket\n",
          getpid() );
}
      else
      {
        buffer[n] = '\0';  /* assuming text.... */
        printf( "CHILD %d: Rcvd message from %s: %s\n",
                getpid(),
                inet_ntoa( (struct in_addr)client.sin_addr ),
                buffer );

        /* send ack message back to the client */
        n = send( newsock, "ACK", 3, 0 );
fflush( NULL );
        if ( n != 3 )
        {
          perror( "send() failed" );
        }
      }
}
while ( n > 0 );
/* this do..while loop exits when the recv() call
   returns 0, indicating the remote/client side has
   closed its socket */

      printf( "CHILD %d: Bye!\n", getpid() );
      close( newsock );
      exit( EXIT_SUCCESS );  /* child terminates here! */

      /* TO DO: add code to handle zombies! */
    }
    else /* pid > 0   PARENT */
    {
      /* parent simply closes the socket (endpoint) */
      close( newsock );
    }
  }

  close( sock );

  return EXIT_SUCCESS;
}