/* matrix summation using pthreads

features: uses a barrier; the Worker[0] computes
the total sum from partial sums computed by Workers
and prints the total sum to the standard output

usage under Linux:
gcc matrixSum.c -lpthread
a.out size numWorkers

*/
#ifndef _REENTRANT
#define _REENTRANT
#endif
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>
#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 10   /* maximum number of workers */

int numWorkers;           /* number of workers */
int numArrived = 0;       /* number who have arrived */
pthread_mutex_t lock;     /*Lock*/

typedef struct {
  long value;
  long i;
  long j;
} Index;

/* timer */
double read_timer() {
  static bool initialized = false;
  static struct timeval start;
  struct timeval end;
  if( !initialized )
  {
    gettimeofday( &start, NULL );
    initialized = true;
  }
  gettimeofday( &end, NULL );
  return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

double start_time, end_time; /* start and end times */
int size, stripSize;  /* assume size is multiple of numWorkers */
int sums[MAXWORKERS]; /* partial sums */
int matrix[MAXSIZE][MAXSIZE]; /* matrix */

Index minIndex;
Index maxIndex;
long sum = 0;

void *Worker(void *);

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
  int i, j;
  long l; /* use long in case of a 64-bit system */
  pthread_attr_t attr;
  pthread_t workerid[MAXWORKERS];
  maxIndex.value = LONG_MIN;
  minIndex.value = LONG_MAX;

  /* set global thread attributes */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  /* initialize mutex variable */
  pthread_mutex_init(&lock, NULL);

  /* read command line args if any */
  size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
  if (size > MAXSIZE) size = MAXSIZE;
  if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
  stripSize = size/numWorkers;

  srand(time(NULL));
  /* initialize the matrix */
  for (i = 0; i < size; i++) {
    for (j = 0; j < size; j++) {
      matrix[i][j] = rand()%99; //1
    }
  }

  /* print the matrix */
  #ifdef DEBUG
  for (i = 0; i < size; i++) {
    printf("[ ");
    for (j = 0; j < size; j++) {
      printf(" %d", matrix[i][j]);
    }
    printf(" ]\n");
  }
  #endif

  /* do the parallel work: create the workers */
  start_time = read_timer();
  for (l = 0; l < numWorkers; l++)
  pthread_create(&workerid[l], &attr, Worker, (void *) l);
  for(l = 0; l < numWorkers; l++){
    pthread_join(workerid[l], NULL);
  }
  /* Get end time */
  end_time = read_timer();

  printf("\nThe total time id %ld", sum);
  printf("\nMax: %ld (%ld %ld)\nMin: %ld (%ld %ld)\n", maxIndex.value, maxIndex.i, maxIndex.j, minIndex.value, minIndex.i, minIndex.j);
  printf("\nThe execution time is %g sec\n", end_time - start_time);

  pthread_exit(NULL);
}

/* Each worker sums the values in one strip of the matrix.
After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg) {
  long myid = (long) arg;
  int total, i, j, first, last;
  Index max_index, min_index;
  max_index.value = LONG_MIN;
  min_index.value = LONG_MAX;

  #ifdef DEBUG
  printf("worker %ld (pthread id %ld) has started\n", myid, (long)pthread_self());
  #endif

  /* determine first and last rows of my strip */
  first = myid*stripSize;
  last = (myid == numWorkers - 1) ? (size - 1) : (first + stripSize - 1);

  /* sum values in my strip */
  total = 0;
  for (i = first; i <= last; i++){
    for (j = 0; j < size; j++){
      total += matrix[i][j];

      if(matrix[i][j] > max_index.value){
        max_index.value = matrix[i][j];
        max_index.i = i;
        max_index.j = j;
      }

      if(matrix[i][j] < min_index.value){
        min_index.value = matrix[i][j];
        min_index.i = i;
        min_index.j = j;
      }
    }
  }
  pthread_mutex_lock(&lock);
  sum += total;
  pthread_mutex_unlock(&lock);

  if(max_index.value > maxIndex.value){
    pthread_mutex_lock(&lock);
    maxIndex.value = max_index.value;
    maxIndex.i = max_index.i;
    maxIndex.j = max_index.j;
    pthread_mutex_unlock(&lock);
  }

  if(min_index.value < minIndex.value){
    pthread_mutex_lock(&lock);
    minIndex.value = min_index.value;
    minIndex.i = min_index.i;
    minIndex.j = min_index.j;
    pthread_mutex_unlock(&lock);
  }

  pthread_exit(NULL);
}
