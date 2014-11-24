/* client.c */

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#define BUFFER_SIZE 5000000

int read_cli(int argc,char const *argv[]){
  if(argc != 2){
    return -1;
  }
  int temp = (unsigned short) atoi(argv[1]);
  if(temp < 8000 || temp > 9000){
    return -1;
  }
  else{
    return temp;
  }
}

void block_server(int * sock){
    char buff[ BUFFER_SIZE ];
    int n;
    do{
      n = read( *sock, buff, BUFFER_SIZE );  // BLOCK
    printf("gets past first send\n");
      if ( n < 0 )
      {
        perror( "read() failed" );
        exit( EXIT_FAILURE );
      }
      else
      {
        buff[n] = '\0';
        printf( "%s", buff );
      }

    }while( n > 0);
    

}

int main(int argc, char const *argv[])
{
  unsigned short port;
  
  //get Port Number from command line
  int tport=read_cli(argc,argv);

  //confirming that portnumber is valid
  if(tport < 0){
    fprintf(stderr, "Usage: file_name port_number(8000-9000)\n");
    exit( EXIT_FAILURE);
  }
  port = (unsigned short) tport;
  /* create TCP client socket (endpoint) */
  int sock = socket( PF_INET, SOCK_STREAM, 0 );

  if ( sock < 0 )
  {
    perror( "socket() failed" );
    exit( EXIT_FAILURE );
  }

  struct hostent * hp = gethostbyname( "localhost" );
  if ( hp == NULL )
  {
    perror( "gethostbyname() failed" );
    exit( EXIT_FAILURE );
  }

  struct sockaddr_in server;
  server.sin_family = PF_INET;
  memcpy( (void *)&server.sin_addr, (void *)hp->h_addr,
          hp->h_length );
  server.sin_port = htons( port );

  printf( "server address is %s\n", inet_ntoa( server.sin_addr ) );

  if ( connect( sock, (struct sockaddr *)&server,
                sizeof( server ) ) < 0 )
  {
    perror( "connect() failed" );
    exit( EXIT_FAILURE );
  }

  char * msg_LIST = "LIST\n\0";
  char * msg_ADD = "ADD maybe3.txt 27\nadsab cdefghijklmnopqrstuvwxyz";
  char * msg_ADD1 = "ADD test.txt 69\nabcdefghijklmnopqrstuvwxyzdksjahfklsdjhfasdjklhfsdklafhasklfhasjklf\0";
  char * msg_APPEND = "APPEND maybe.txt 20\nAPPENDED: HELLO WORLD";
  char * msg_READ = "READ test.txt\n\0";
  char * msg_DELETE = "DELETE test.txt\n\0";
  char * msg_ADD_binary = "ADD banner1.png 59930\n";
  int n;
  struct stat buf;

  //adding binary file to server
  char sdbuf[BUFFER_SIZE];
  bzero(sdbuf, BUFFER_SIZE); 
  int fs_block_sz; 
  FILE * fs = fopen("banner1.png","r");
  while((fs_block_sz = fread(sdbuf, sizeof(char), BUFFER_SIZE, fs)) > 0)
  {
      if(send(sock, sdbuf, fs_block_sz, 0) < 0)
      {
          break;
      }
      bzero(sdbuf, BUFFER_SIZE);
  }

  char revbuf[BUFFER_SIZE];
  char* fr_name = "banner2.png";
  FILE *fr = fopen(fr_name, "w");
  if(fr == NULL)
    printf("File %s Cannot be opened.\n", fr_name);
  else
  {
    bzero(revbuf, BUFFER_SIZE); 
    int fr_block_sz = 0;
    while((fr_block_sz = recv(sock, revbuf, BUFFER_SIZE, 0)) > 0)
    {
      int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);
      if(write_sz < fr_block_sz)
      {
        error("File write failed.\n");
      }
      bzero(revbuf, BUFFER_SIZE);
      if (fr_block_sz == 0 || fr_block_sz != 512) 
      {
        break;
      }
    }
    if(fr_block_sz < 0)
      printf("recv() timed out.\n");
  }
  printf("Ok received from server!\n");
  fclose(fr);

/*  
  //Check LIST Command
  printf("Sending %s\n", msg_LIST);
  n = write( sock, msg_LIST, strlen( msg_LIST ) );
  fflush( NULL );
  if ( n < strlen( msg_LIST ) )
  {
    perror( "write() failed" );
    exit( EXIT_FAILURE );
  }
  block_server(&sock);
*/

 /* //Check ADD Command
  printf("Sending %s\n", msg_ADD);
  n = write( sock, msg_ADD, strlen( msg_ADD ) );
  fflush( NULL );
  if ( n < strlen( msg_ADD ) )
  {
    perror( "write() failed" );
    exit( EXIT_FAILURE );
  }
  block_server(&sock);

  //Check LIST Command again
  printf("Sending %s\n", msg_LIST);
  n = write( sock, msg_LIST, strlen( msg_LIST ) );
  fflush( NULL );
  if ( n < strlen( msg_LIST ) )
  {
    perror( "write() failed" );
    exit( EXIT_FAILURE );
  }
  block_server(&sock);


  //Check SECOND ADD COMMAND
  printf("Sending %s\n", msg_ADD);
  n = write( sock, msg_ADD1, strlen( msg_ADD1 ) );
  fflush( NULL );
  if ( n < strlen( msg_ADD1 ) )
  {
    perror( "write() failed" );
    exit( EXIT_FAILURE );
  }
  block_server(&sock);

  //Check LIST Command again
  printf("Sending %s\n", msg_LIST);
  n = write( sock, msg_LIST, strlen( msg_LIST ) );
  fflush( NULL );
  if ( n < strlen( msg_LIST ) )
  {
    perror( "write() failed" );
    exit( EXIT_FAILURE );
  }
  block_server(&sock);
*/
/*
  //check APPEND Command
  printf("Sending %s\n", msg_APPEND);
  n = write( sock, msg_APPEND, strlen( msg_APPEND ) );
  fflush( NULL );
  if ( n < strlen( msg_APPEND ) )
  {
    perror( "write() failed" );
    exit( EXIT_FAILURE );
  }
  block_server(&sock);
*/
/*
  //Check READ Command 
  printf("Sending %s\n", msg_READ);
  n = write( sock, msg_READ, strlen( msg_READ ) );
  fflush( NULL );
  if ( n < strlen( msg_READ ) )
  {
    perror( "write() failed" );
    exit( EXIT_FAILURE );
  }
  block_server(&sock);
*/
/*
  //Check DELETE Command
  printf("Sending %s\n", msg_DELETE);
  n = write( sock, msg_DELETE, strlen( msg_DELETE ) );
  fflush( NULL );
  if ( n < strlen( msg_DELETE ) )
  {
    perror( "write() failed" );
    exit( EXIT_FAILURE );
  }
  block_server(&sock);

  //Check LIST Command again
  printf("Sending %s\n", msg_LIST);
  n = write( sock, msg_LIST, strlen( msg_LIST ) );
  fflush( NULL );
  if ( n < strlen( msg_LIST ) )
  {
    perror( "write() failed" );
    exit( EXIT_FAILURE );
  }
  block_server(&sock);
*/  
/*
  //check add binary file
  n = write( sock, binary_file, strlen( binary_file ) );
  fflush( NULL );
  if ( n < strlen( binary_file ) )
  {
    perror( "write() failed" );
    exit( EXIT_FAILURE );
  }
  block_server(&sock);
*/

  close( sock );
  return EXIT_SUCCESS;
}

