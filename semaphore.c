/* semaphore.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#define NUM_THREADS 20

void * thread_code( void * in );

int total;

int main()
{
  int i;
  int in[NUM_THREADS];  /* the inputs to each thread */

  pthread_t thread_id[NUM_THREADS];  /* the thread IDs */

  total = 1;

  for ( i = 0 ; i < NUM_THREADS ; i++ )
  {
    in[i] = i;
  }

  /* initialize the semaphore to 1 (binary semaphore) */
  sem_init( &mutex, 0, 1 );

  for ( i = 0 ; i < NUM_THREADS ; i++ )
  {
    pthread_create( &thread_id[i], NULL,
                    (void *)&thread_code, (void *)&in[i] );
  }

  for ( i = 0 ; i < NUM_THREADS ; i++ )
  {
    pthread_join( thread_id[i], NULL );
  }

  /* once all threads are done, we can destroy semaphore */
  sem_destroy( &mutex );

  return EXIT_SUCCESS;
}

void * thread_code( void * in )
{
  int x = *( (int *)in );

  printf( "THREAD #%d: waiting to enter critical section\n", x );

  sem_wait( &mutex );   /******  P(mutex)  ******/

  /******  CRITICAL SECTION  ******/
  printf( "THREAD #%d: entering critical section\n", x );
  int tmp = total;
  printf( "THREAD #%d: doubling total from %d...\n", x, tmp );
  tmp = tmp * 2;
  printf( "THREAD #%d: ...to %d\n", x, tmp );
  total = tmp;
  printf( "THREAD #%d: exiting critical section\n", x );
  /******  CRITICAL SECTION  ******/

  sem_post( &mutex );   /******  V(mutex)  ******/

  pthread_exit( NULL );
  return (void *)NULL;
}