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
#include <dirent.h>

#define BUFFER_SIZE 5000000
#define NUM_THREADS 20
#define STORAGE ".storage"
#define FILE_CAPACITY 1000
//Global variables
sem_t tLock;
int fileCount;
struct thread_lock{
  char filename[sizeof(char)*100];
  char used;
  sem_t readLock;
  sem_t editLock;
} locks[FILE_CAPACITY];

struct thread_input{
    int fd;
}; 

//Function Prototypes
int read_cli(int argc,char const *argv[]);
int check_lstat(char *cmd);
int check_dir(char *cmd);
void initalize_threads(pthread_t *thread_id, int *inputs);
void * start_routine( struct thread_input *input );
void exec_LIST(int *clientSockID);
void exec_READ(int *clientSockID, char *file_name);
void exec_DELETE(int *clientSockID, char *file_name);
void exec_ADD(int *clientSockID, char *file_name, int *bytes, char *file_contents);
void exec_APPEND(int *clientSockID, char *file_name, int *bytes, char *file_contents);
void lock_init(void);
void fill_locks(void);

//Server Implementation
int main(int argc, char const *argv[]){
  unsigned short port;
  int i;
	printf("Started file-server\n");
	
	//get Port Number from command line
	int tport=read_cli(argc,argv);

	//confirming that portnumber is valid
	if(tport < 0){
		fprintf(stderr, "Usage: file_name port_number(8000-9000)\n");
		exit( EXIT_FAILURE);
	}
  port = (unsigned short) tport;

  //initializing thread lock
  sem_init( &tLock, 0, 1 );
  fileCount=0;
  sem_wait( &tLock );
  lock_init();
  sem_post( &tLock );

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
    fill_locks();
  }

  //Create Threads
  pthread_t thread_id[NUM_THREADS];
  struct thread_input inputs[NUM_THREADS];
  
  for( i=0; i<NUM_THREADS; ++i){
    inputs[i].fd=-1;
    pthread_create( &thread_id[i], NULL, (void *)&start_routine, &inputs[i] );
  }

  //server loop
  i=0;
  while(1){

    //Accepted Client Connection
    int clientSock = accept( sock, (struct sockaddr *)&client,(socklen_t*)&clientLength );
    printf("Received incoming connection from %s\n", inet_ntoa( (struct in_addr)client.sin_addr ));

    //reset i to 0 if NUM_THREADS reached
    //avoids starvation of threads
    if(i == NUM_THREADS-1){
      i=0;
    }
    //select +  assign client to a thread
    for( ; i < NUM_THREADS; ++i){
      
      //reset i to 0 if NUM_THREADS reached
      //avoids starvation of threads
      if(i == NUM_THREADS-1){
        i=0;
      }
      if(inputs[i].fd == -1){
        inputs[i].fd = clientSock;
        break;
      }
    }

  }

  //Destroy Semaphores
  sem_destroy( &tLock );
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

void lock_init( void ){
  int i;
  for( i=0; i < FILE_CAPACITY; ++i){
    strcpy(locks[i].filename,"\0");
    locks[i].used = '0';
    sem_init( &locks[i].readLock, 0, NUM_THREADS );
    sem_init( &locks[i].editLock, 0, 1 );
  }
}

void fill_locks(void){
  DIR *dirp;
  struct dirent entry;
  struct dirent *result = NULL;
  int n;
  if ((dirp = opendir (".storage")) != NULL) {
    do{
      n=readdir_r(dirp, &entry, &result);
      if( n != 0){
        perror( "readdir_r() error" );
        return;
      }
      if( entry.d_type & DT_REG ){
        sem_wait( &tLock );
        strcpy(locks[fileCount].filename,entry.d_name);
        locks[fileCount].used = '1';
        sem_post( &tLock );
        ++fileCount;
      }
    }while( result != NULL);
  }
}


//return 0 if text, and return 1 if it is binary
int check_lstat(char *cmd){
  struct stat buf;
  int rc = lstat( cmd, &buf );
    if(rc == 0){
      if ( S_ISREG( buf.st_mode ) ){
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

void * start_routine( struct thread_input * input ){
  //variable initialzations
  int clientSockID;
  char *cmd=(char *)malloc(sizeof(char)*BUFFER_SIZE);
  char *file_name=(char *)malloc(sizeof(char)*BUFFER_SIZE);
  char *byte_string=(char *)malloc(sizeof(char)*BUFFER_SIZE);
  char *file_contents=(char *)malloc(sizeof(char)*BUFFER_SIZE);
  char * buffer=(char *)malloc(sizeof(char)*BUFFER_SIZE);
  char * temp=(char *)malloc(sizeof(char)*BUFFER_SIZE);
  int bytes;
  
  while(1){
    clientSockID = input->fd;

    //wait until thread is assigned to a client
    if(clientSockID == -1){
      continue;
    }

    //perform actions until messageLength is == 0
    int messageLength;
    do{
      //variable clen-up
      bzero(cmd,sizeof(char)*BUFFER_SIZE);
      bzero(file_name,sizeof(char)*BUFFER_SIZE);
      bzero( byte_string , sizeof(char)*BUFFER_SIZE);
      bzero( file_contents , sizeof(char)*BUFFER_SIZE);
      bzero(buffer,sizeof(char)*BUFFER_SIZE);
      bzero(temp,sizeof(char)*BUFFER_SIZE);
      messageLength = recv( clientSockID, buffer, BUFFER_SIZE, 0 );

      //Check recv() Success        
      if ( messageLength < 0 ){
        perror( "recv() failed" );
        return NULL;
      }

      //Socket Closing
      else if ( messageLength == 0 ){
        printf( "[thread %u] Client closed its socket....terminating\n", (unsigned int) pthread_self() );
      }

      //Perform command
      else{

        //parsing of client message (stored in buffer)
        strcpy(temp,buffer);
        cmd = strtok(temp, " \n");
        if( strcmp(cmd,"LIST\0") == 0 ){
          printf( "[thread %u] Rcvd: %s\n", (unsigned int) pthread_self(), cmd);
          exec_LIST( &clientSockID );
        }
        else if ( strcmp(cmd, "DELETE\0")  == 0 ){
          file_name = strtok( NULL, "\n" );
          printf( "[thread %u] Rcvd: %s %s\n", (unsigned int) pthread_self(), cmd, file_name);
          exec_DELETE( &clientSockID, file_name );
        }
        else if( strcmp(cmd, "READ\0")  == 0 ){
          file_name = strtok( NULL, "\n" );
          printf( "[thread %u] Rcvd: %s %s\n", (unsigned int) pthread_self(), cmd, file_name);
          exec_READ( &clientSockID, file_name );
        }
        else if( strcmp(cmd, "ADD\0")  == 0 || strcmp(cmd, "APPEND\0")  == 0 ){
            file_name = strtok( NULL, " " );
            byte_string = strtok( NULL, "\n" );
            bytes = atoi( byte_string );
            file_contents = byte_string + strlen(byte_string) + 1;
            if( strcmp( cmd, "ADD\0" ) == 0){
              printf( "[thread %u] Rcvd: %s %s %d\n", (unsigned int) pthread_self(), cmd, file_name, bytes);
              exec_ADD( &clientSockID, file_name, &bytes, file_contents );
            }
            else{
              printf( "[thread %u] Rcvd: %s %s %d\n", (unsigned int) pthread_self(), cmd, file_name, bytes);
              exec_APPEND( &clientSockID, file_name, &bytes, file_contents );
            }
        }
        else{
          printf( "[thread %u] Rcvd: %s\n", (unsigned int) pthread_self(), cmd);
          printf("[thread %u] SENT: ERROR: INVALID COMMAND\n", (unsigned int) pthread_self());
          int sendLength;
          char * msg = "ERROR: INVALID COMMAND\n";
          sendLength = send( clientSockID, msg , strlen(msg), 0);
          fflush(NULL);
          if( sendLength != strlen(msg)){
              perror( "send() failed");
              return NULL;
          }
        }
        
      }
      
    }while( messageLength > 0 );

    close( clientSockID );
    input->fd = -1;
    
  }
  free( cmd );
  free( file_name );
  free( byte_string );
  free( file_contents );
  free( buffer );
  free( temp );
  pthread_exit( NULL );
  return (void *)NULL;
}

void exec_LIST(int *clientSockID){
/*LIST\n
  -- server returns the list of files currently stored on the server
  -- the list need not be in any specific order
  -- format of message containing list of files is as follows:

        <number-of-files>\n<filename1>\n<filename2>\netc.\n

  -- if no files are stored, "0\n" is returned

   send ack message back to the client 
  
   int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result);
*/
  DIR *dirp;
  struct dirent entry;
  struct dirent *result = NULL;
  int n, sendLength, count=0;
  //getting file number (variable count)
  if ((dirp = opendir (".storage")) != NULL) {
    n=readdir_r(dirp, &entry, &result);
    if( n == 0){
      do{

        if( entry.d_type & DT_REG ){
          count += 1;
        }
        
        //read next file file_name
        n=readdir_r(dirp, &entry, &result);
        if( n != 0){
          perror( "readdir_r() error" );
          return;
        }

        //handle no file case
        if(count == 0 && result == NULL){
          sendLength = send( *clientSockID, "0\n", 2, 0);
          fflush(NULL);
          if( sendLength != 2){
            perror( "send() failed");
            return;
          }
          return;        
        }

        if( result == NULL ){
          char num_files[15];
          sprintf( num_files, "%d",count );
          sendLength = send( *clientSockID, num_files, strlen(num_files), 0);
          fflush(NULL);
          if( sendLength != strlen(num_files) ){
            perror( "send() failed");
            return;
          }
          sendLength = send( *clientSockID, "\n", 1, 0);
          fflush(NULL);
          if( sendLength != 1){
            perror( "send() failed");
            return;
          }
          printf("[thread %u] SENT: %d\n", (unsigned int) pthread_self(), count);
        }

      }while(result != NULL);
    closedir (dirp);
    }
  }
  else {
    /* could not open directory */
    perror ("opendir()  error");
    return;
  }

  //actual file sending
  if ((dirp = opendir (".storage")) != NULL) {
    n=readdir_r(dirp, &entry, &result);
    if( n == 0){
      do{
        if( entry.d_type & DT_REG ){
          //send first message
          sendLength = send( *clientSockID, entry.d_name , strlen(entry.d_name), 0);
          fflush(NULL);
          if( sendLength != strlen(entry.d_name)){
              perror( "send() failed");
              return;
          }
          sendLength = send( *clientSockID, "\n" , 1, 0);
          fflush(NULL);
          if( sendLength != 1){
              perror( "send() failed");
              return;
          }
          printf("[thread %u] SENT: %s\n", (unsigned int) pthread_self(),entry.d_name);
        }
        
        //read next file file_name
        n=readdir_r(dirp, &entry, &result);
        if( n != 0){
          perror( "readdir_r() error" );
          return;
        }

      }while(result != NULL);
    closedir (dirp);
    }
  }
 
  else {
    /* could not open directory */
    perror ("opendir()  error");
    return;
  }

}
void exec_READ(int *clientSockID, char *file_name){
/*READ <filename>\n
-- server returns the length (in bytes) and content of <filename>
-- note that this does NOT remove the file on the server
-- if the file does not exist, return an "ERROR: NO SUCH FILE\n" error
-- return "ACK" if successful
-- return "ERROR: <error-description>\n" if unsuccessful
-- if "ACK" is sent, follow it with file length and file contents, as follows:

      ACK <bytes>\n<file-contents>
*/
  int sendLength;
  char * file = ".storage/";
  char* path;
  path = malloc(strlen(file)+strlen(file_name)+1);
  bzero(path,strlen(file)+strlen(file_name)+1);
  strcpy(path, file);
  strcat(path, file_name);
  struct stat buf;

  //check if file exists
  int rc = lstat( path, &buf );
  
  //if file exists, send info + contents
  if(rc == 0 && S_ISREG( buf.st_mode ) ){
    
    //send ACK +  byte size
    sendLength = send( *clientSockID, "ACK " , 4, 0);
    fflush(NULL);
    if( sendLength != 4 ){
        perror( "send() failed");
        return;
    }
    char num_files[15];
    int s=(int)buf.st_size;
    sprintf( num_files, "%d",s );
    sendLength = send( *clientSockID, num_files , strlen(num_files) , 0);
    fflush(NULL);
    if( sendLength != strlen(num_files) ){
        perror( "send() failed");
        return;
    }
    sendLength = send( *clientSockID, "\n" , 1 , 0);
    fflush(NULL);
    if( sendLength != 1 ){
        perror( "send() failed");
        return;
    }
    printf("[thread %u] SENT: ACK %s\n", (unsigned int) pthread_self(),num_files);

    int index;
    for( index=0; index < fileCount; ++index){
      if(locks[index].used == '1'){
        if( strcmp(locks[index].filename,file_name) == 0 ){
          sem_wait( &locks[index].readLock );
          break;
        }
      }
    }
    FILE *f_stream = fopen(path,"r");
    char buffer[s+1];
    int bytes_read = 0;
    int cur_bytes;
    while( bytes_read < s){
      cur_bytes = fread( buffer, 1, s, f_stream);
      bytes_read += cur_bytes;
      sendLength = send( *clientSockID, buffer , cur_bytes, 0);
      fflush(NULL);
      if( sendLength != cur_bytes){
          perror( "send() failed");
          return;
      }
      sendLength = send( *clientSockID, "\n" , 1, 0);
      fflush(NULL);
      if( sendLength != 1 ){
          perror( "send() failed");
          return;
      }
    }
    fclose(f_stream);
    printf("[thread %u] SENT: Transferred file (%d bytes)\n", (unsigned int) pthread_self(),s);
    sem_post( &locks[index].readLock );
  }
  else{
    char * msg = "ERROR: NO SUCH FILE\n";
    sendLength = send( *clientSockID, msg , strlen(msg), 0);
    fflush(NULL);
    if( sendLength != strlen(msg)){
        perror( "send() failed");
        return;
    }
    printf("[thread %u] SENT: %s", (unsigned int) pthread_self(),msg);  
  }
}

void exec_DELETE(int *clientSockID, char *file_name){
/*DELETE <filename>\n
-- delete file <filename> from the storage server
-- if the file does not exist, return an "ERROR: NO SUCH FILE\n" error
-- return "ACK" if successful
-- return "ERROR: <error-description>\n" if unsuccessful
*/

  int sendLength;
  char * file = ".storage/";
  char* path;
  path = malloc(strlen(file)+strlen(file_name)+1);
  bzero(path,strlen(file)+strlen(file_name)+1);
  strcpy(path, file);
  strcat(path, file_name);
  struct stat buf;

  //check if file exists
  int rc = lstat( path, &buf );
  
  //if file exists, send info + contents
  if(rc == 0 && S_ISREG( buf.st_mode ) ){

    //gaining rights to DELETE
    int index,sval;
    for( index=0; index < fileCount; ++index){
      if(locks[index].used == '1'){
        if( strcmp(locks[index].filename,file_name) == 0 ){
          sem_wait( &locks[index].editLock );
          break;
        }
      }
    }

    do{
      sem_getvalue( &locks[index].readLock, &sval);
    }while( sval != NUM_THREADS );
    sem_wait( &tLock );

    //removing file at path
    if(remove(path) == 0){
      printf("[thread %u] Delete \"%s\" file\n", (unsigned int) pthread_self(),file_name);
      locks[index].used='0';
      strcpy(locks[index].filename,"\0");
      --fileCount;

      //send ACK on success
      sendLength = send( *clientSockID, "ACK\n" , 4, 0);
      fflush(NULL);
      if( sendLength != 4 ){
          perror( "send() failed");
          return;
      }
      printf("[thread %u] SENT: ACK\n", (unsigned int) pthread_self());
    }

    sem_post( &locks[index].editLock );
    sem_post( &tLock );
  }
  else{
    char * msg = "ERROR: NO SUCH FILE\n";
    sendLength = send( *clientSockID, msg , strlen(msg), 0);
    fflush(NULL);
    if( sendLength != strlen(msg)){
        perror( "send() failed");
        return;
    }  
    printf("[thread %u] SENT: %s", (unsigned int) pthread_self(),msg);  
  }
}
void exec_ADD(int *clientSockID, char *file_name, int *bytes, char *file_contents){
/*  ADD <filename> <bytes>\n<file-contents>
-- add <filename> to the storage server
-- if the file already exists, return an "ERROR: FILE EXISTS\n" error
-- return "ACK" if successful
-- return "ERROR: <error-description>\n" if unsuccessful
*/

  int sendLength;
  char * file = ".storage/";
  char* path;
  path = malloc(strlen(file)+strlen(file_name)+1);
  bzero(path,strlen(file)+strlen(file_name)+1);
  strcpy(path, file);
  strcat(path, file_name);
  struct stat buf;

  //check if file exists
  int rc = lstat( path, &buf );
  
  //if file already exists
  if(rc == 0){
    char * msg = "ERROR: FILE EXISTS\n";
    sendLength = send( *clientSockID, msg , strlen(msg), 0);
    fflush(NULL);
    if( sendLength != strlen(msg)){
        perror( "send() failed");
        return;
    }  
    printf("[thread %u] SENT: %s", (unsigned int) pthread_self(),msg);
  }
  //file needs to be created
  else{

    int index;
    //initialize file locks & increment fileCount
    if(fileCount < FILE_CAPACITY){
      ++fileCount;

      for( index=0; index < fileCount; ++index){
        if(locks[index].used == '0'){
          sem_wait( &tLock );
          locks[index].used='1';
          strcpy(locks[index].filename,file_name);
          sem_post( &tLock );
          break;
        }
      }    
    }
    else{
      char * errorMessage = "ERROR: file storage capacity reached\n";
      sendLength = send( *clientSockID, errorMessage , strlen(errorMessage), 0);
      fflush(NULL);
      if( sendLength != strlen(errorMessage)){
        perror( "send() failed");
        return;
      } 
      printf("[thread %u] SENT: %s", (unsigned int) pthread_self(),errorMessage);  
    }

    sem_wait( &locks[index].editLock );

    //creating file at path
    FILE * f_stream = fopen(path,"w");
    if(f_stream != NULL){
      
      int z;
      z = fwrite(file_contents, 1, strlen(file_contents), f_stream);
      if( z == 0){
        char * msg = "ERROR: couldn't write to file (on server)\n";
        sendLength = send( *clientSockID, msg , strlen(msg), 0);
        fflush(NULL);
        if( sendLength != strlen(msg)){
          perror( "send() failed");
          return;
        }
        printf("[thread %u] SENT: %s", (unsigned int) pthread_self(),msg); 
        perror( "fwrite() failed" );
        return;
      }
      printf("[thread %u] Transferred file (%d bytes)\n", (unsigned int) pthread_self(),strlen(file_contents));

      //send ACK on success
      sendLength = send( *clientSockID, "ACK\n" , 4, 0);
      fflush(NULL);
      if( sendLength != 4 ){
          perror( "send() failed");
          return;
      }
      printf("[thread %u] SENT: ACK\n", (unsigned int) pthread_self());
    }
    //file failed to be created
    else{
      char *temp = "ERROR: file creation failed\n";
      sendLength = send( *clientSockID, temp , strlen(temp), 0);
      fflush(NULL);
      if( sendLength != strlen(temp) ){
          perror( "send() failed");
          return;
      } 
      perror( "fopen() failed");
      printf("[thread %u] SENT: %s", (unsigned int) pthread_self(),temp);
      return;
    }
    fclose(f_stream);

    sem_post( &locks[index].editLock );
  }

}

void exec_APPEND(int *clientSockID, char *file_name, int *bytes, char *file_contents){
/*  APPEND <filename> <bytes>\n<file-contents>
-- append <filename> to the storage server by
    adding <file-contents> to the given file
-- if the file does not exist, return an "ERROR: NO SUCH FILE\n" error
-- return "ACK" if successful
-- return "ERROR: <error-description>\n" if unsuccessful
*/

  int sendLength;
  char * file = ".storage/";
  char* path;
  path = malloc(strlen(file)+strlen(file_name)+1);
  bzero(path,strlen(file)+strlen(file_name)+1);
  strcpy(path, file);
  strcat(path, file_name);
  struct stat buf;

  //check if file exists
  int rc = lstat( path, &buf );

  //if file exists, append
  if(rc == 0 ){
    int index,sval;
    for( index=0; index < fileCount; ++index){
      if(locks[index].used == '1'){
        if( strcmp(locks[index].filename,file_name) == 0 ){
          sem_wait( &tLock );
          break;
        }
      }
    }

    do{
      sem_getvalue( &locks[index].readLock, &sval);
    }while( sval != NUM_THREADS );
    
    sem_wait( &locks[index].editLock );
    //creating file at path
    FILE * f_stream = fopen(path,"a");
    if(f_stream != NULL){
      
      int z;
      z = fwrite(file_contents, 1, strlen(file_contents), f_stream);
      if( z == 0){
        char * msg = "ERROR: couldn't write to file (on server)\n";
        sendLength = send( *clientSockID, msg , strlen(msg), 0);
        fflush(NULL);
        if( sendLength != strlen(msg)){
          perror( "send() failed");
          return;
        }
        printf("[thread %u] SENT: %s", (unsigned int) pthread_self(),msg); 
        perror( "fwrite() failed" );
        return;
      }
      printf("[thread %u] Appended to file \"%s\" (%d bytes)\n", (unsigned int) pthread_self(), file_name, *bytes);
      //send ACK on success
      sendLength = send( *clientSockID, "ACK\n" , 4, 0);
      fflush(NULL);
      if( sendLength != 4 ){
          perror( "send() failed");
          return;
      }
      printf("[thread %u] SENT: ACK\n", (unsigned int) pthread_self());
    }
    //file failed to be created
    else{
      char *temp = "ERROR: file creation failed\n";
      sendLength = send( *clientSockID, temp , strlen(temp), 0);
      fflush(NULL);
      if( sendLength != strlen(temp) ){
          perror( "send() failed");
          return;
      } 
      printf("[thread %u] SENT: %s", (unsigned int) pthread_self(),temp);
      perror( "fopen() failed");
      return;
    }
    fclose(f_stream);
    sem_post( &tLock );
    sem_post( &locks[index].editLock );
  }
  //file doesnt exist!
  else{
    char * msg = "ERROR: NO SUCH FILE\n";
    sendLength = send( *clientSockID, msg , strlen(msg), 0);
    fflush(NULL);
    if( sendLength != strlen(msg)){
        perror( "send() failed");
        return;
    }
    printf("[thread %u] SENT: %s", (unsigned int) pthread_self(),msg);
  }    

}