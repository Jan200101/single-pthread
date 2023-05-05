#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

#include "single-pthread.h"

#ifndef __x86_64
#error TODO
#endif

#ifndef REG_RIP
#error broken
#endif

#define INSTRUCTION_POINTER_REGISTER REG_RIP
#define ARG1_POINTER_REGISTER REG_RDI
#define TIMER_INTERVAL 1

static green_thread* thread_list = NULL;
static size_t thread_list_size = 0;
static size_t thread_index = 0;

static void thread_trampoline(green_thread* thread)
{
    thread->retval = thread->routine(thread->arg);

    thread->running = 0;
    thread->done = 1;
    sleep(-1);
}

static void setup_timer()
{
    int ret;
    //fprintf(stderr, "[*] setting up timer\n");

    struct sigevent clock_sig_event;
    memset(&clock_sig_event, 0, sizeof( struct sigevent));
    clock_sig_event.sigev_notify = SIGEV_SIGNAL;
    clock_sig_event.sigev_signo = SIGUSR1;
    clock_sig_event.sigev_notify_attributes = NULL;

    timer_t timer_id;
    /* Creating process interval timer */
    ret = timer_create(CLOCK_MONOTONIC, &clock_sig_event, &timer_id);
    assert(ret == 0);

    struct itimerspec timer_value;

    /* settings timer interval values */
    memset(&timer_value, 0, sizeof(struct itimerspec));
    timer_value.it_interval.tv_sec = TIMER_INTERVAL;
    timer_value.it_interval.tv_nsec = 0;

    /* setting timer initial expiration values*/
    timer_value.it_value.tv_sec = TIMER_INTERVAL;
    timer_value.it_value.tv_nsec = 0;

    /* Create timer */
    ret = timer_settime(timer_id, 0, &timer_value, NULL);
    assert(ret == 0);
}

static void signal_handler(int signum, siginfo_t* info, void*ptr) {
    if (!thread_list)
        return;

    green_thread* cur_thread = &thread_list[thread_index];
    green_thread* next_thread;

    while(1)
    {
        thread_index = (thread_index+1) % thread_list_size;

        next_thread = &thread_list[thread_index];

        if (next_thread->done)
            continue;   

        break;
    }

    if (cur_thread == next_thread)
    {
        //fprintf(stderr, "[*] No new threads found, continuing\n");
        return;
    }

    //fprintf(stderr, "[*] Switching context to %ld\n", thread_index);

    ucontext_t *ucontext = (ucontext_t*)ptr;

    memcpy(cur_thread->gregs, ucontext->uc_mcontext.gregs, sizeof(gregset_t));

    if (next_thread->running)
    {
        //fprintf(stderr, "[*] Continuing process\n");
        // thread is already running, just continue it
        memcpy(ucontext->uc_mcontext.gregs, next_thread->gregs, sizeof(gregset_t));
    }
    else
    {
        //fprintf(stderr, "[*] Starting process\n");
        ucontext->uc_mcontext.gregs[INSTRUCTION_POINTER_REGISTER] = (greg_t)thread_trampoline;
        ucontext->uc_mcontext.gregs[ARG1_POINTER_REGISTER] = (greg_t)next_thread;
    }

    return;
}

static void setup_signal()
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = signal_handler;
    action.sa_flags = SA_SIGINFO;
    if(sigaction(SIGUSR1, &action, NULL) < 0)
        perror("sigaction");
}

static void setup()
{
    setup_signal();
    setup_timer();
}

static void init_thread_list()
{
    ++thread_list_size;

    thread_list = realloc(thread_list, thread_list_size * sizeof(green_thread));
    green_thread* entry = &thread_list[thread_list_size-1];

    memset(entry, 0, sizeof(*entry));
    entry->running = 1;
}

int pthread_create(pthread_t *restrict thread,
                    const void *restrict attr,
                    pthread_routine_t start_routine,
                    void *restrict arg)
{
    if (!thread_list) {
        setup();
        init_thread_list();
    }

    *thread = ++thread_list_size;
    thread_list = realloc(thread_list, thread_list_size * sizeof(green_thread));

    green_thread* entry = &thread_list[thread_list_size-1];

    memset(entry, 0, sizeof(green_thread));
    entry->routine = start_routine;
    entry->arg = arg;
    entry->running = 0;
    entry->done = 0;

    return 0;
}
