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


void* print_thread(void* arg)
{
    size_t num = (size_t)arg;
    printf("thread num: %ld\n", num);

    return NULL;
}

int main()
{
    pthread_t threads[5];
    for (size_t i =  0; i < 5; i++)
    {
        pthread_create(&threads[i], NULL, print_thread, (void*)i);
    }

    puts("sleeping");
    sleep(10); // Should run 1-5
    puts("Sleep some more");
    sleep(10); // Nothing should print here
    puts("Done sleeping");
}