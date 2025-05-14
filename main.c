#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <memory.h>
#include <syscall.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <time.h>

#include "single-pthread.h"


#define THREAD_COUNT 10

void* print_thread(void* arg)
{
    size_t num = (size_t)arg;
    size_t run_count = 0;
    while (run_count < 10)
    {
        printf("running thread %zu for the %zu time\n", num, run_count);
        run_count++;
        sleep(1);
    }

    return NULL;
}

int main()
{
    pthread_t threads[THREAD_COUNT];
    for (size_t i =  0;i < THREAD_COUNT; i++)
    {
        pthread_create(&threads[i], NULL, print_thread, (void*)i + 1);
    }

    puts("sleeping");
    sleep(10); // Should run 1-THREAD_COUNT
    puts("Done");
}