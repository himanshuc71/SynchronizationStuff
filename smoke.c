#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "uthread.h"
#include "uthread_mutex_cond.h"

#define NUM_ITERATIONS 1000

#ifdef VERBOSE
#define VERBOSE_PRINT(S, ...) printf (S, ##__VA_ARGS__);
#else
#define VERBOSE_PRINT(S, ...) ;
#endif

struct Agent {
    uthread_mutex_t mutex;
    uthread_cond_t match;
    uthread_cond_t paper;
    uthread_cond_t tobacco;
    uthread_cond_t smoke;
};

struct Agent *createAgent() {
    struct Agent *agent = malloc(sizeof(struct Agent));
    agent->mutex = uthread_mutex_create();
    agent->paper = uthread_cond_create(agent->mutex);
    agent->match = uthread_cond_create(agent->mutex);
    agent->tobacco = uthread_cond_create(agent->mutex);
    agent->smoke = uthread_cond_create(agent->mutex);
    return agent;
}

struct Middleman {
    struct Agent *agent;
    uthread_cond_t match_paper;
    uthread_cond_t match_tobacco;
    uthread_cond_t paper_tobacco;
    int paper;
    int match;
    int tobacco;
};

struct Middleman* createMiddleman(void* av){
    struct Agent* a = av;
    struct Middleman* middleman = malloc (sizeof(struct Middleman));
    middleman->agent= av;
    middleman->match_paper = uthread_cond_create(a->mutex);
    middleman->match_tobacco = uthread_cond_create(a->mutex);
    middleman->paper_tobacco = uthread_cond_create(a->mutex);
    middleman->paper = 0;
    middleman->match = 0;
    middleman->tobacco = 0;
}

//
// TODO
// You will probably need to add some procedures and struct etc.
//

/**
 * You might find these declarations helpful.
 *   Note that Resource enum had values 1, 2 and 4 so you can combine resources;
 *   e.g., having a MATCH and PAPER is the value MATCH | PAPER == 1 | 2 == 3
 */
enum Resource {
    MATCH = 1, PAPER = 2, TOBACCO = 4
};
char *resource_name[] = {"", "match", "paper", "", "tobacco"};

int signal_count[5];  // # of times resource signalled
int smoke_count[5];  // # of times smoker with resource smoked


// match resource 1
void* smokerMatch(void *mv) {
    struct Middleman *m = mv;
    uthread_mutex_lock(m->agent->mutex);
    while(1){

        uthread_cond_wait(m->paper_tobacco);
        smoke_count[MATCH]++;
        m->paper--;
        m->tobacco--;
        VERBOSE_PRINT ("smoker with match smoked\n");
        uthread_cond_signal(m->agent->smoke);

    }

}
// paper resource 2
void* smokerPaper(void *mv) {
    struct Middleman *m = mv;
    uthread_mutex_lock(m->agent->mutex);
    while(1){

        uthread_cond_wait(m->match_tobacco);
        smoke_count[PAPER]++;
        m->match--;
        m->tobacco--;
        VERBOSE_PRINT ("smoker with paper smoked\n");
        uthread_cond_signal(m->agent->smoke);

    }

}
// tobacco resource 4
void* smokerTobacco(void *mv) {
    struct Middleman *m = mv;
    uthread_mutex_lock(m->agent->mutex);
    while(1){

        uthread_cond_wait(m->match_paper);
        smoke_count[TOBACCO]++;
        m->match--;
        m->paper--;
        VERBOSE_PRINT ("smoker with tobacco smoked\n");
        uthread_cond_signal(m->agent->smoke);

    }
}

void* middleManMatch(void *mv){
    struct  Middleman *m = mv;
    uthread_mutex_lock(m->agent->mutex);
    while (1){
            VERBOSE_PRINT ("waiting for match signal from agent\n");
            uthread_cond_wait(m->agent->match);
            m->match++;
            VERBOSE_PRINT ("match added\n");

    }
}

void* middleManPaper(void *mv){
    struct  Middleman *m = mv;
    uthread_mutex_lock(m->agent->mutex);
    while(1){

            VERBOSE_PRINT ("waiting for paper signal from agent\n");
            uthread_cond_wait(m->agent->paper);
            m->paper++;
            VERBOSE_PRINT ("paper added \n");

    }
}

void* middleManTobacco(void *mv){
    struct  Middleman *m = mv;
    uthread_mutex_lock(m->agent->mutex);
    while(1){


            VERBOSE_PRINT ("waiting for tobacco signal from agent\n");
            uthread_cond_wait(m->agent->tobacco);
            m->tobacco++;
            VERBOSE_PRINT ("tobacco added\n");

    }
}

void* middleManHead(void* mv){
    struct  Middleman *m = mv;

    while(1) {
        uthread_mutex_lock(m->agent->mutex);
        if (m->match > 0 && m->paper > 0) {
            VERBOSE_PRINT ("signaled tobacco smoker\n");
            uthread_cond_signal(m->match_paper);
        }

        else if (m->match > 0 && m->tobacco > 0) {
            VERBOSE_PRINT ("signaled paper smoker\n");
            uthread_cond_signal(m->match_tobacco);
        }

        else if (m->paper > 0 && m->tobacco > 0) {
            VERBOSE_PRINT ("signaled match smoker\n");
            uthread_cond_signal(m->paper_tobacco);
        }
        uthread_mutex_unlock(m->agent->mutex);
    }
}
/**
 * This is the agent procedure.  It is complete and you shouldn't change it in
 * any material way.  You can re-write it if you like, but be sure that all it does
 * is choose 2 random reasources, signal their condition variables, and then wait
 * wait for a smoker to smoke.
 */


void *agent(void *av) {
    struct Agent *a = av;
    static const int choices[] = {MATCH | PAPER, MATCH | TOBACCO, PAPER | TOBACCO};
    static const int matching_smoker[] = {TOBACCO, PAPER, MATCH};

    uthread_mutex_lock(a->mutex);
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        int r = random() % 3;
        signal_count[matching_smoker[r]]++;
        int c = choices[r];
        if (c & MATCH) { VERBOSE_PRINT ("match available\n");
            uthread_cond_signal(a->match);
        }
        if (c & PAPER) { VERBOSE_PRINT ("paper available\n");
            uthread_cond_signal(a->paper);
        }
        if (c & TOBACCO) { VERBOSE_PRINT ("tobacco available\n");
            uthread_cond_signal(a->tobacco);
        }VERBOSE_PRINT ("agent is waiting for smoker to smoke\n");
        uthread_cond_wait(a->smoke);
    }
    uthread_mutex_unlock(a->mutex);
    return NULL;
}

int main(int argc, char **argv) {
    uthread_init(7);
    struct Agent *a = createAgent();
    struct Middleman* m = createMiddleman(a);
    // TODO
    uthread_create(middleManMatch, m);
    uthread_create(middleManPaper, m);
    uthread_create(middleManTobacco, m);
    uthread_create(middleManHead, m);
    uthread_create(smokerMatch, m);
    uthread_create(smokerPaper, m);
    uthread_create(smokerTobacco, m);


    uthread_join(uthread_create(agent, a), 0);
    assert(signal_count[MATCH] == smoke_count[MATCH]);
    assert(signal_count[PAPER] == smoke_count[PAPER]);
    assert(signal_count[TOBACCO] == smoke_count[TOBACCO]);
    assert(smoke_count[MATCH] + smoke_count[PAPER] + smoke_count[TOBACCO] == NUM_ITERATIONS);
    printf("Smoke counts: %d matches, %d paper, %d tobacco\n",
           smoke_count[MATCH], smoke_count[PAPER], smoke_count[TOBACCO]);
}