#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"
#include "spinlock.h"

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;
uthread_mutex_t m;
uthread_cond_t isMax, isZero;

void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO
      uthread_mutex_lock(m);
      while (items == MAX_ITEMS){
          producer_wait_count++;
          uthread_cond_wait(isMax);
      }
      items++;
      histogram[items]++;
      uthread_cond_broadcast(isZero);
      uthread_mutex_unlock(m);
  }
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO
      uthread_mutex_lock(m);
      while (items == 0){
          consumer_wait_count++;
          uthread_cond_wait(isZero);
      }
      items--;
      histogram[items]++;
      uthread_cond_broadcast(isMax);
      uthread_mutex_unlock(m);
  }
  return NULL;
}

int main (int argc, char** argv) {
  uthread_t t[4];

  uthread_init (4);
  m = uthread_mutex_create();
  isMax = uthread_cond_create(m);
  isZero = uthread_cond_create(m);

  // TODO: Create Threads and Join
    for (int j = 0; j <4 ; j++) {
        if (j%2==0)
            t[j]= uthread_create(producer,NULL);
        else
            t[j]= uthread_create(consumer,NULL);
    }
    for (int k = 0; k <4 ; k++) {
        uthread_join(t[k],NULL);
    }
  
  printf ("producer_wait_count=%d, consumer_wait_count=%d\n", producer_wait_count, consumer_wait_count);
  printf ("items value histogram:\n");
  int sum=0;
  for (int i = 0; i <= MAX_ITEMS; i++) {
    printf ("  items=%d, %d times\n", i, histogram [i]);
    sum += histogram [i];
  }
  assert (sum == sizeof (t) / sizeof (uthread_t) * NUM_ITERATIONS);
}
