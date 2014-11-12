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
	if(portNum < 0){
		fprintf(stderr, "Usage: file_name port_number(8000-9000)\n");
		exit(1);
	}
	
	//starting port connection
	printf("Listening on Port %d\n", portNum);

	//run server, checking to assure it successfuly runs
	if(run(portNum) != 0){
		exit(1);
	}


	printf("Exiting File Storage Server Simulation\n");
	return 0;
}

//read command line arguments, return port number
//if invalid, return -1
int read_cli(int argc,char const *argv[]){
	if(argc != 2){
		return -1;
	}
	int temp = atoi(argv[1]);
	if(temp < 8000 || temp > 9000){
		return -1;
	}
	else{
		return temp;
	}
}

int run(int portNum){
	


	return 0;
}

//checks command and calls if valid
check_command(char * cmd){
	ADD <filename> <bytes>\n<file-contents>
	APPEND <filename> <bytes>\n<file-contents>
	READ <filename>\n
	LIST\n
	DELETE <filename>\n
}