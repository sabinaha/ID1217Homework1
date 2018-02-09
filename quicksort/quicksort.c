#ifndef _REENTRANT
#define _REENTRANT
#endif
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 10   /* maximum number of workers */

//Global variables
int numWorkers;           /* number of workers */
long size;  /* */
pthread_t workerid[MAXWORKERS];

//Function declaration
void *quicksort(void *);
int partition(int *, int, int);
void swap(int *, int *);
double read_timer();
void printArray(int *);
void seq_qsort(int *, int, int);

typedef struct{
  long id;
  int *arr;
  int lo;
  int hi;
} Work_Args;

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

void printArray(int *arr){
  long i;
  printf("[ ");
  for(i = 0; i < size; i++){
    printf("%d, ", arr[i]);
  }
  printf(" ]\n");
}

void swap(int *a, int *b){
  int temp = *a;
  *a = *b;
  *b = temp;
}

void seq_qsort(int *arr, int lo, int hi){
  if(lo < hi){
    int pLocation = partition(arr, lo, hi);
    seq_qsort(arr, lo, pLocation);
    seq_qsort(arr, pLocation+1, hi);
  }
}

void *quicksort(void *arg){
  Work_Args *args = (Work_Args *) arg;
  long myId = args->id;
  int* arr = args->arr;
  int lo = args->lo;
  int hi = args->hi;

  int pLocation = partition(arr, lo, hi);

  if(myId*2 + 1 > MAXWORKERS){
    seq_qsort(arr, lo, pLocation);
    seq_qsort(arr, pLocation+1, hi);
  }
  else if(lo < hi){

    //Making attributes for threads
    pthread_attr_t attribute1;
    pthread_attr_init(&attribute1);
    pthread_attr_t attribute2;
    pthread_attr_init(&attribute2);

    //Create the arguments to pass along to new wrkers
    Work_Args args1 = (Work_Args) {.id = myId*2, .arr = arr, .lo = lo, .hi = pLocation};
    Work_Args args2 = (Work_Args) {.id = myId*2 + 1, .arr = arr, .lo = pLocation+1, .hi = hi};

    pthread_create(&workerid[myId*2], &attribute1, quicksort, (void *) &args1);
    pthread_create(&workerid[myId*2+1], &attribute2, quicksort, (void *) &args2);
    pthread_join(workerid[myId*2], (void *) &arr);
    pthread_join(workerid[myId*2 + 1], (void *) &arr);
  }
  pthread_exit((void * ) arr);
}

int partition(int *arr, int lo, int hi){
  int i;
  int p = arr[lo];
  int lWall = lo;

  for(i = lo+1; i < hi; i++){
    if(arr[i] < p){
      swap(&arr[i], &arr[lWall+1]);
      lWall++;
    }
  }

  swap(&arr[lo], &arr[lWall]);

  return lWall;
}

int main(int argc, char *argv[]) {
  long i;

  //Initialize sizes
  size = (argc > 1) ? size = atoi(argv[1]) : MAXSIZE;
  size = (size > MAXSIZE) ? MAXSIZE : size;

  //workers -1 due to using heap structure and therefore skipping index 0
  numWorkers = (argc > 2) ? numWorkers = atoi(argv[2]) : MAXWORKERS - 1;
  numWorkers = (numWorkers >= MAXWORKERS) ? MAXWORKERS - 1 : numWorkers;

  printf("==== RUN INFO ====\n");
  printf("Size:\t%ld\nNum workers:\t%d\n\n", size, numWorkers);

  //Malloc space for answer
  int *ans = calloc(size, sizeof(int));

  //Thread start
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  //Test data
  srand(time(NULL));
  for(i = 0; i < size; i++){
    ans[i] = rand() % 100 + 1;
  }

  #ifdef DEBUG
    printf("Unsorted array: \n");
    printArray(ans);
  #endif

  //Create the arguments for the first worker
  Work_Args args = (Work_Args) {.id = 1, .arr = ans, .lo = 0, .hi = size};

  //Start timers and the workers
  double start_time = read_timer();

  pthread_create(&workerid[1], &attr, quicksort, (void * ) &args);
  pthread_join(workerid[1], (void * ) &ans);

  double end_time = read_timer();

  printf("Execution time: %f\n", end_time - start_time);

  #ifdef DEBUG
    printf("Sorted array:\n");
    printArray(ans);
  #endif

  free(ans);

  pthread_exit(NULL);
}
