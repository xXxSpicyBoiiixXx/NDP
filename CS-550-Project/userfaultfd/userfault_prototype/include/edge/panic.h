#pragma once

#ifdef _WIN32

#include <processthreadsapi.h>
#include <stdio.h>
#include <stdlib.h>

#define PANIC(msg, ...) \
    do { \
        fprintf(stderr, "panic! (tid %ld) [%s] %s:L%d: " msg "\n", \
                GetCurrentThreadId(), \
                __FILE__,  \
                __FUNCTION__, \
                __LINE__, \
                __VA_ARGS__); \
        fflush(stderr); \
        getchar(); \
        exit(1); \
    } while (0)

#else

#include <pthread.h>
#include <cstdlib>

#define PANIC(msg, vargs...) \
    do { \
        fprintf(stderr, "panic! (tid %d) %s:L%d in %s: " msg "\n", \
                (int32_t) pthread_self(), \
                __FILE__,  \
                __LINE__, \
                __FUNCTION__, \
                ##vargs); \
        fflush(stderr); \
        exit(1); \
    } while (0)

#endif

#define PANIC_IF_NULL(_VAR) \
  do { \
    if (_VAR == NULL) { PANIC("`%s` cannot be null", "#_VAR#"); } \
  } while (0)
