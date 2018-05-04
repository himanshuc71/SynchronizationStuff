#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "uthread.h"
#include "uthread_sem.h"

#define MAX_ITEMS 10
const int NUM_ITERATIONS = 200;
const int NUM_CONSUMERS  = 2;
const int NUM_PRODUCERS  = 2;

int producer_wait_count;     // # of times producer had to wait
int consumer_wait_count;     // # of times consumer had to wait
int histogram [MAX_ITEMS+1]; // histogram [i] == # of times list stored i items

int items = 0;

uthread_sem_t lock;
uthread_sem_t numberOfItems;
uthread_sem_t itemsRemaining;
uthread_sem_t done;


void* producer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO
    uthread_sem_wait(itemsRemaining);
    uthread_sem_wait(lock);
    items++;
    histogram[items]++;
    uthread_sem_signal(lock);
    uthread_sem_signal(numberOfItems);

  }
  uthread_sem_signal(done);
  return NULL;
}

void* consumer (void* v) {
  for (int i=0; i<NUM_ITERATIONS; i++) {
    // TODO
    uthread_sem_wait(numberOfItems);
    uthread_sem_wait(lock);
    items--;
    histogram[items]++;
    uthread_sem_signal(lock);
    uthread_sem_signal(itemsRemaining);
  }
  uthread_sem_signal(done);
  return NULL;
}

int main (int argc, char** argv) {
  uthread_t t[4];

  uthread_init (4);


  lock = uthread_sem_create(1);
  itemsRemaining = uthread_sem_create(MAX_ITEMS);
  numberOfItems = uthread_sem_create(0);
  done = uthread_sem_create(0);

  // TODO: Create Threads and Join
  for (int j = 0; j <NUM_CONSUMERS + NUM_PRODUCERS ; j++) {
    if (j%2==0)
      t[j]= uthread_create(producer,NULL);
    else
      t[j]= uthread_create(consumer,NULL);
  }

  for (int k = 0; k <NUM_CONSUMERS + NUM_PRODUCERS ; k++) {
    uthread_sem_wait(done);
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
