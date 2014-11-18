/* octuplets-threads-mutex.c */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

/* gcc ....  -lpthread */
#include <pthread.h>

#define CHILDREN 8

void * whattodo( void * arg );

/* global mutex variable */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main()
{
  int i, rc;
  int * t;
  pthread_t tid[ CHILDREN ];

  /* Create threads */
  for ( i = 0 ; i < CHILDREN ; i++ )
  {
    t = (int *)malloc( sizeof( int ) );
    *t = 3 + ( rand() % 11 );   /* in range [3,13] */
    printf( "MAIN: Next thread will nap for %d seconds.\n", *t );

    rc = pthread_create( &tid[i], NULL, whattodo, t );
    if ( rc != 0 ) {
      perror( "Could not create the thread" );
    }
  }

  /* Wait for threads to terminate */
  for ( i = 0 ; i < CHILDREN ; i++ ) {
    pthread_join( tid[i], NULL );       /* BLOCKING */
  }

  printf( "MAIN: All threads accounted for.\n" );
  return 0;
}

void critical_section( int nap_time )
{
  printf( "THREAD %u: Entering my critical section.\n", (unsigned int)pthread_self() );
  printf( "THREAD %u: Napping for %d seconds.\n", (unsigned int)pthread_self(), nap_time );
  sleep( nap_time );
  printf( "THREAD %u: Leaving my critical section.\n", (unsigned int)pthread_self() );
}

void * whattodo( void * arg )
{
  int t = *(int *)arg;
  printf( "THREAD %u: I'm want to nap for %d seconds.\n", (unsigned int)pthread_self(), t );

  pthread_mutex_lock( &mutex );
    critical_section( t );
  pthread_mutex_unlock( &mutex );

  printf( "THREAD %u: I'm awake now.\n", (unsigned int)pthread_self() );
  return NULL;
}




