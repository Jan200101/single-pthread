#ifndef SINGLE_PTHREAD_H
#define SINGLE_PTHREAD_H

#include <sys/user.h>
#include <ucontext.h>
#include <unistd.h>
#include <syscall.h>
#include <stdlib.h>
#include <stdint.h>

/**
 * signal.h tries to import pthread types, don't let it
 */
#ifdef __USE_POSIX199506 // No pthread_t
#undef __USE_POSIX199506
#endif
#ifdef __USE_UNIX98 // No pthread_t
#undef __USE_UNIX98
#endif
#ifndef __USE_MISC // For sigreturn
#define __USE_MISC
#endif
#include <signal.h>

#ifdef SIGSTKSZ
#undef SIGSTKSZ
#endif

#define SIGSTKSZ 8192

#define inline_kill(pid, signal) \
    {{ \
        ssize_t ret; \
        asm volatile \
        ( \
            "syscall" \
            : "=a" (ret) \
            : "0"(SYS_kill), "D"(pid), "S"(signal) \
            : "rcx", "r11", "memory" \
        ); \
    }}

#define inline_raise(signal) inline_kill(getpid(), signal)

typedef void *(*pthread_routine_t)(void*);

#ifndef SIGSTKSZ
#error BLA
#endif

typedef struct {
    pthread_routine_t routine;
    void* arg;
    void* retval;
    ucontext_t ctx;
    uint8_t running;
    uint8_t done;
    char stack[SIGSTKSZ];
} green_thread;

int pthread_create(pthread_t *restrict thread,
                    const void *restrict attr,
                    pthread_routine_t start_routine,
                    void *restrict arg);

#endif