/* main.c */

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
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024
#define NUM_THREADS 20
#define STORAGE ".storage"

//Global Semaphores
sem_t addLock;
sem_t appendLock;
sem_t readLock;
sem_t deleteLock;
sem_t lock;

//Function Prototypes
int read_cli(int argc,char const *argv[]);
int check_lstat(char *cmd);
int check_dir(char *cmd);
void initalize_threads(pthread_t *thread_id, int *inputs);
void * start_routine( void * inputs );

//Server Implementation
int main(int argc, char const *argv[]){
  unsigned short port;
	printf("Started file-server\n");
	
	//get Port Number from command line
	int tport=read_cli(argc,argv);

	//confirming that portnumber is valid
	if(tport < 0){
		fprintf(stderr, "Usage: file_name port_number(8000-9000)\n");
		exit( EXIT_FAILURE);
	}
  port = (unsigned short) tport;

  //Creating Listener Socket (TCP)
  int sock = socket( AF_INET, SOCK_STREAM, 0 );

  //Check Socket creted successfully
  if ( sock < 0 ){
    perror( "socket() failed" );
    exit( EXIT_FAILURE );
  }
  
  //Socket Structures
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;

  /* htons() is host-to-network-short for marshalling */
  /* Internet is "big endian"; Intel is "little endian" */
  server.sin_port = htons( port );
  int serverLength = sizeof( server );

  //Binding name to socket
  if ( bind( sock, (struct sockaddr *)&server, serverLength ) < 0 ){
    perror( "bind() failed" );
    exit( EXIT_FAILURE );
  }

	//Listener is bound to port and listening
  listen( sock, NUM_THREADS );
	printf("Listening on Port %d\n", port);

  //Client Address Created
  struct sockaddr_in client;
  int clientLength = sizeof( client );

  //Set-Up .storage directory
  if(check_dir(STORAGE) == 1){
    int rc = mkdir(STORAGE, S_IRWXU | S_IRWXG | S_IROTH );
    if(rc != 0){
      perror( "mkdir() failed ");
      exit( EXIT_FAILURE );
    }
    printf("Directory '.storage' created.\n");
  }
  else{
    printf("Directory '.storage' already exists.\n");
  }

  //Initialize Semaphores
  sem_init( &addLock, 0, 1 );
  sem_init( &appendLock, 0, 1 );
  sem_init( &readLock, 0, NUM_THREADS );
  sem_init( &deleteLock, 0, 1 );
  sem_init( &lock, 0 ,1 );

  //Create Threads
  pthread_t thread_id[NUM_THREADS];
  int inputs[NUM_THREADS];  /* the inputs to each thread */
  int i;
  for( i=0; i<NUM_THREADS; ++i){
    inputs[i]=i;
    pthread_create( &thread_id[i], NULL, (void *)&start_routine, (void *)&inputs[i] );
  }

  //input variables/file descriptors
  int pid;
  char buffer[ BUFFER_SIZE ];
  // FILE *file;

  //server loop
  while(1){

    //Accepted Client Connection
    int clientSock = accept( sock, (struct sockaddr *)&client,(socklen_t*)&clientLength );
    printf("Received incoming connection from %s\n", inet_ntoa( (struct in_addr)client.sin_addr ));

    //Client Handling: Child Processes
    pid = fork();

    //check for fork errors
    if ( pid < 0 ){
      perror( "fork() failed" );
      exit( EXIT_FAILURE );
    }

    //Child process
    //To-Do: implement commands + file server
    else if ( pid == 0){
      
      //perform actions until messageLength is == 0
      int messageLength;
      do{
        messageLength = recv( clientSock, buffer, BUFFER_SIZE, 0 );

        //Check recv() Success        
        if ( messageLength < 0 ){
          perror( "recv() failed" );
        }

        //Socket Closing
        else if ( messageLength == 0 ){
          printf( "[thread %d] Client closed its socket....terminating\n",
                  getpid() );
        }

        //To-Do: Perform command
        else{
          buffer[messageLength] = '\0';
          printf( "[thread %d] Rcvd %s\n", getpid(),buffer);

          //To-Do: implement commands

          /* send ack message back to the client */
          int sendLength;
          //ssize_t send(int sockfd, const void *buf, size_t len, int flags);
          sendLength = send( clientSock, "ACK\n", 4 , 0 );
          fflush( NULL );
          if ( sendLength != 4 ){
            perror( "send() failed" );
          }
        }
      }while( messageLength > 0 );

      close( clientSock );
      exit( EXIT_SUCCESS );

      //To-Do: Handle Zombie Processes
    }

    //Parent Process
    else{
      close( clientSock );
    }

  }

	printf("Exiting File Storage Server Simulation\n");
  close( sock );

	return EXIT_SUCCESS;
}

//read command line arguments, return port number
//if invalid, return -1
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

//return 0 if text, and return 1 if it is binary
int check_lstat(char *cmd){
  struct stat buf;
  int rc = lstat( cmd, &buf );
    if(rc == 0){
      if ( S_ISREG( buf.st_mode ) && buf.st_mode & ( S_IXUSR | S_IXGRP | S_IXOTH )){
      return 0;
      }
    }

  return 1;
}

//return 0 if it is a directory
int check_dir(char *cmd){
  struct stat buf;
  int rc = lstat( cmd, &buf );
    if(rc == 0){
      if ( S_ISDIR( buf.st_mode )){
      return 0;
      }
    }

  return 1; 
}

void * start_routine( void * inputs ){

  pthread_exit( NULL );
  return (void *)NULL;
}
