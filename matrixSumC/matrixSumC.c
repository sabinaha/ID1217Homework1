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

pthread_mutex_t sumLock;     /*Lock*/
pthread_mutex_t maxLock;
pthread_mutex_t minLock;
pthread_mutex_t rowLock;
int numWorkers;           /* number of workers */

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
int size;  /* assume size is multiple of numWorkers */
int matrix[MAXSIZE][MAXSIZE]; /* matrix */
int nextRow;

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

  /* set global thread attributes */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  /* initialize mutex variable */
  pthread_mutex_init(&sumLock, NULL);
  pthread_mutex_init(&maxLock, NULL);
  pthread_mutex_init(&minLock, NULL);
  pthread_mutex_init(&rowLock, NULL);

  /* read command line args if any */
  size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
  if (size > MAXSIZE){
    size = MAXSIZE;
  }
  if (numWorkers > MAXWORKERS){
    numWorkers = MAXWORKERS;
  }

  srand(time(NULL));
  /* initialize the matrix */
  for (i = 0; i < size; i++) {
    for (j = 0; j < size; j++) {
      matrix[i][j] = rand()%99;
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

  nextRow = 0;
  maxIndex.value = LONG_MIN;
  minIndex.value = LONG_MAX;

  /*Read timer fort start time*/
  start_time = read_timer();

  /* do the parallel work: create the workers */
  for (l = 0; l < numWorkers; l++){
    pthread_create(&workerid[l], &attr, Worker, (void *) l);
  }

  for(l = 0; l < numWorkers; l++){
    pthread_join(workerid[l], NULL);
  }

  /* Get end time */
  end_time = read_timer();

  /* Print results */
  printf("\nTotal sum: %ld", sum);
  printf("\nMax: %ld (%ld %ld)\nMin: %ld (%ld %ld)\n", maxIndex.value, maxIndex.i, maxIndex.j, minIndex.value, minIndex.i, minIndex.j);
  printf("\nThe execution time is %g sec\n", end_time - start_time);

  pthread_exit(NULL);
}

/* Each worker sums the values in one strip of the matrix.
After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg) {
  long myid = (long) arg;
  int total, row, j;
  Index max_index, min_index;

  #ifdef DEBUG
  printf("worker %ld (pthread id %ld) has started\n", myid, (long)pthread_self());
  #endif

  while(true){

    min_index.value = LONG_MAX;
    max_index.value = LONG_MIN;
    total = 0;

    pthread_mutex_lock(&rowLock);
    row = nextRow++;
    pthread_mutex_unlock(&rowLock);
    if(nextRow > size){
      break;
    }
    /* sum values in my strip */
    for (j = 0; j < size; j++){
      total += matrix[row][j];
      if(matrix[row][j] > max_index.value){
        max_index.value = matrix[row][j];
        max_index.i = row;
        max_index.j = j;
      }
      if(matrix[row][j] < min_index.value){
        min_index.value = matrix[row][j];
        min_index.i = row;
        min_index.j = j;
      }
    }


    pthread_mutex_lock(&sumLock);
    sum += total;
    pthread_mutex_unlock(&sumLock);

    if(max_index.value > maxIndex.value){
      pthread_mutex_lock(&maxLock);
        maxIndex.value = max_index.value;
        maxIndex.i = max_index.i;
        maxIndex.j = max_index.j;
      pthread_mutex_unlock(&maxLock);
    }
    if(min_index.value < minIndex.value){
      pthread_mutex_lock(&minLock);
        minIndex.value = min_index.value;
        minIndex.i = min_index.i;
        minIndex.j = min_index.j;
      pthread_mutex_unlock(&minLock);
    }
  }

  #ifdef DEBUG
  printf("worker %ld (pthread id %ld) has finished\n", myid, (long)pthread_self());
  #endif

  pthread_exit(NULL);
}
