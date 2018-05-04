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
spinlock_t lock;

void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO
      spinlock_lock(&lock);
      while (items == MAX_ITEMS){
          spinlock_unlock(&lock);
          producer_wait_count++;
          spinlock_lock(&lock);
      }
      items++;
      histogram[items]++;
      spinlock_unlock(&lock);
  }
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO
      spinlock_lock(&lock);
      while (items == 0){
          spinlock_unlock(&lock);
          consumer_wait_count++;
          spinlock_lock(&lock);
      }
      items--;
      histogram[items]++;
      spinlock_unlock(&lock);
  }
  return NULL;
}

int main (int argc, char** argv) {
  uthread_t t[NUM_PRODUCERS+NUM_CONSUMERS];

  uthread_init (4);
  spinlock_create(&lock);
  
  // TODO: Create Threads and Join
    for (int j = 0; j <NUM_PRODUCERS+NUM_CONSUMERS ; j++) {
        if (j%2==0)
            t[j]= uthread_create(producer,NULL);
        else
            t[j]= uthread_create(consumer,NULL);
    }
    for (int k = 0; k <NUM_PRODUCERS+NUM_CONSUMERS ; k++) {
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
