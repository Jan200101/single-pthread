#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

#include "single-pthread.h"

#define TIMER_INTERVAL_MS 100

static green_thread* thread_list = NULL;
volatile static size_t thread_list_size = 0;
volatile static size_t thread_index = 0;

//#define DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#define DEBUG_PRINT(...)

static void thread_trampoline(green_thread* thread)
{
    thread->running = 1;
    thread->done = 0;

    thread->retval = thread->routine(thread->arg);

    thread->running = 0;
    thread->done = 1;
    sleep(-1);
}

static void setup_timer()
{
    int ret;
    DEBUG_PRINT("[*] setting up timer\n");

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
    timer_value.it_interval.tv_sec = (TIMER_INTERVAL_MS / 1000);
    timer_value.it_interval.tv_nsec = (TIMER_INTERVAL_MS % 1000) * 1000000;

    /* setting timer initial expiration values*/
    timer_value.it_value.tv_sec = (TIMER_INTERVAL_MS / 1000);
    timer_value.it_value.tv_nsec = (TIMER_INTERVAL_MS % 1000) * 1000000;

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
        DEBUG_PRINT("[*] No new threads found, continuing\n");
        return;
    }

    DEBUG_PRINT("[*] Switching context to %ld\n", thread_index);

    ucontext_t *ucontext = (ucontext_t*)ptr;

    memcpy(&cur_thread->ctx, ucontext, sizeof(*ucontext));

    if (next_thread->running)
    {
        DEBUG_PRINT("[*] Continuing process\n");
        // thread is already running, just continue it
        memcpy(ucontext, &next_thread->ctx, sizeof(*ucontext));
    }
    else
    {
        DEBUG_PRINT("[*] Starting process\n");
        ucontext->uc_stack.ss_sp = cur_thread->stack;
        ucontext->uc_stack.ss_size = SIGSTKSZ;
        makecontext(ucontext, (void (*)(void))thread_trampoline, 1, next_thread);
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

static void handle_exit(void)
{
    DEBUG_PRINT("[!] in exit handler\n");

    if (!thread_list)
    {
        DEBUG_PRINT("[+] No threads found.\n");
        return;
    }

    while (true)
    {
        size_t index = thread_index;
        if (index == 0) ++index;
        if (thread_list[index].done) break;

        DEBUG_PRINT("[*] thread %zu still running (%i), waiting\n", index, thread_list[index].done);
        sleep(-1);
    }
}

static void setup()
{
    setup_signal();
    setup_timer();

    atexit(handle_exit);
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
