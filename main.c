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
#include <assert.h>

//Function Prototypes
int read_cli(int argc,char const *argv[]);

//Server Implementation
int main(int argc, char const *argv[])
{
	int portNum;
	printf("Started file-server\n");
	
	//get Port Number from command line
	portNum=read_cli(argc,argv);

	//confirming that portnumber is valid
	assert(portNum > 0);
	
	//starting port connection
	printf("Listening on Port %d\n", portNum);

	return 0;
}

//read command line arguments, return port number
//if invalid, return -1
int read_cli(int argc,char const *argv[]){
	if(argc != 2){
		fprintf(stderr, "ERROR: Invald Number of Command Line Arguments.\n");
		return -1;
	}
	int temp = atoi(argv[1]);
	if(temp < 0){
		fprintf(stderr, "ERROR: Invalid Port Number Entered.\n");
		return -1;
	}
	else{
		return temp;
	}
}
