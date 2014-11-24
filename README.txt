Operating Systems HW #3
Author: Chris May

In my main.c file, I implement a server which can receive the LIST, ADD, DELETE, APPEND, and READ commands. 

To handle multiple client connections, I created a pool of n number of threads, defined by the NUM_THREADS pre-processor definition. These threads hold a struct which contains the current file descriptor. If the descriptor is negative one it continues checking until the server acssigns it an accepted client fd. 

The server avoids thread starvation by placing the available threads in a queue. A thread resets after an allotted amount of standby time or when the socket is closed. 

 I created a structure to hold all file locks for each file.

 struct thread_lock{
  char filename[sizeof(char)*100];
  char used;
  sem_t readLock;
  sem_t editLock;
}

The readLock ensures is a check for delete to ensure that the current sval is equal to the total number of threads before a file is deleted or edited.
