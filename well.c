#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"
#include <time.h>

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

#define MAX_OCCUPANCY      3
#define NUM_ITERATIONS     100
#define NUM_PEOPLE         20
#define FAIR_WAITING_COUNT 4

/**
 * You might find these declarations useful.
 */
enum Endianness {LITTLE = 0, BIG = 1};
const static enum Endianness oppositeEnd [] = {BIG, LITTLE};

struct Well {
  // TODO
    int occupants;
    enum Endianness occupantType;
    uthread_mutex_t mx;
    uthread_cond_t changeEndianness;
    uthread_cond_t waitWellFull;
    int requestChange;                    // acts as a boolean
};

struct Well* createWell() {
  struct Well* Well = malloc (sizeof (struct Well));
  // TODO
  Well->occupants = 0;
  Well->occupantType = LITTLE;
  Well->mx = uthread_mutex_create();
  Well->changeEndianness = uthread_cond_create(Well->mx);
  Well->waitWellFull = uthread_cond_create(Well->mx);
  Well->requestChange = 0;
  return Well;
}

struct Well* Well;

#define WAITING_HISTOGRAM_SIZE (NUM_ITERATIONS * NUM_PEOPLE)
int             entryTicker;                                          // incremented with each entry
int             waitingHistogram         [WAITING_HISTOGRAM_SIZE];
int             waitingHistogramOverflow;
uthread_mutex_t waitingHistogrammutex;
int             occupancyHistogram       [2] [MAX_OCCUPANCY + 1];

void recordWaitingTime (int waitingTime) {
  uthread_mutex_lock (waitingHistogrammutex);
  if (waitingTime < WAITING_HISTOGRAM_SIZE)
    waitingHistogram [waitingTime] ++;
  else
    waitingHistogramOverflow ++;
  uthread_mutex_unlock (waitingHistogrammutex);
}

void enterWell (enum Endianness g) {
    uthread_mutex_lock(Well->mx);
    if (Well->occupants == 0){
      Well->occupantType = g;
    }
    int waitCount = entryTicker;
    while(Well->occupants == MAX_OCCUPANCY || Well->occupantType != g){
      if(Well->occupantType != g && (entryTicker - waitCount) > FAIR_WAITING_COUNT){
        VERBOSE_PRINT("Person with endiannes %d getting pissed change endianness\n", g);
        Well->requestChange = 1;
        uthread_cond_wait(Well->changeEndianness);
      }
      else {
          VERBOSE_PRINT("Person with endianness %d waiting to enter\n",g);
          if (Well->occupants == 0){
              Well->occupantType = g;
              break;
          }
          uthread_cond_wait(Well->waitWellFull);
      }
    }
    assert(g == Well->occupantType);
    assert(Well->occupants < MAX_OCCUPANCY);
    recordWaitingTime (entryTicker - waitCount);
    entryTicker++;
    Well->occupants++;
    occupancyHistogram[g][Well->occupants]++;
    VERBOSE_PRINT("Person with endianness %d entered\n",g);
    VERBOSE_PRINT("Well has %d people now\n",Well->occupants);
    uthread_mutex_unlock(Well->mx);
}

void leaveWell() {
  uthread_mutex_lock(Well->mx);
  VERBOSE_PRINT("Person with endianness %d leaving\n",Well->occupantType);
  Well->occupants--;
  VERBOSE_PRINT("Well has %d people now\n",Well->occupants);
  if (Well->occupants == 0 && Well->requestChange){
    Well->requestChange = 0;
    Well->occupantType = oppositeEnd[Well->occupantType];
    VERBOSE_PRINT("Well occupant type changed to %d\n", Well->occupantType);
    uthread_cond_broadcast(Well->changeEndianness);
  }
  VERBOSE_PRINT("people with endianness %d can enter\n",Well->occupantType);
  uthread_cond_broadcast(Well->waitWellFull);
  uthread_mutex_unlock(Well->mx);
}

//
// TODO
// You will probably need to create some additional produres etc.
//

void* person () {
  enum Endianness g = random() % 2;
  printf("Person with endianness %d\n",g);
  for (int i=0;i<NUM_ITERATIONS;i++) {
    enterWell(g);
    for (int i=0;i<NUM_PEOPLE;i++) {
      uthread_yield();
    }
    leaveWell();
    for (int i=0;i<NUM_PEOPLE;i++) {
      uthread_yield();
    }
  }
}

int main (int argc, char** argv) {
  uthread_init (1);
  Well = createWell();
  uthread_t pt [NUM_PEOPLE];
  waitingHistogrammutex = uthread_mutex_create ();

  srand(time(NULL));
  for(int i = 0;i < NUM_PEOPLE; i++){
    pt[i] = uthread_create(person, 0);
  }
  for(int i = 0;i < NUM_PEOPLE; i++){
    uthread_join(pt[i],0);
  }

  printf ("Times with 1 little endian %d\n", occupancyHistogram [LITTLE]   [1]);
  printf ("Times with 2 little endian %d\n", occupancyHistogram [LITTLE]   [2]);
  printf ("Times with 3 little endian %d\n", occupancyHistogram [LITTLE]   [3]);
  printf ("Times with 1 big endian    %d\n", occupancyHistogram [BIG] [1]);
  printf ("Times with 2 big endian    %d\n", occupancyHistogram [BIG] [2]);
  printf ("Times with 3 big endian    %d\n", occupancyHistogram [BIG] [3]);
  printf ("Waiting Histogram\n");
  for (int i=0; i<WAITING_HISTOGRAM_SIZE; i++)
    if (waitingHistogram [i])
      printf ("  Number of times people waited for %d %s to enter: %d\n", i, i==1?"person":"people", waitingHistogram [i]);
  if (waitingHistogramOverflow)
    printf ("  Number of times people waited more than %d entries: %d\n", WAITING_HISTOGRAM_SIZE, waitingHistogramOverflow);
}
