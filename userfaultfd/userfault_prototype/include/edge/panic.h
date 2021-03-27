#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#ifndef __USE_GNU
#define __USE_GNU 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define WARN(msg, vargs...) \
    do { \
        fprintf(stderr, "warn: %s:%d in %s: " msg "\n", \
                __FILE__, \
                __LINE__, \
                __FUNCTION__, \
                ##vargs); \
        fflush(stderr); \
    } while (0)

#define PANIC(msg, vargs...) \
    do { \
        fprintf(stderr, "panic! %s:%d in %s: " msg "\n", \
                __FILE__, \
                __LINE__, \
                __FUNCTION__, \
                ##vargs); \
        fflush(stderr); \
        exit(1); \
    } while (0)

#define PANIC_IF_NULL(_VAR) \
    do { \
        if ((_VAR) == NULL) { PANIC("`%s` cannot be null", #_VAR); } \
    } while (0)

#define PANIC_IF_ZERO(_VAR) \
    do { \
        if ((_VAR) == 0) { PANIC("`%s` cannot be zero", #_VAR); } \
    } while (0)

#define ENSURE_OR_PANIC(_VAR) \
    do { \
        if (!(_VAR)) { PANIC("assertion failed: `%s`", #_VAR); } \
    } while (0)

#define PANIC_IF_NEG(_VAR) \
    do { \
        if ((_VAR) < 0) { PANIC("`%s` cannot be negative", "#_VAR#"); } \
    } while (0)

#define WARN_WITH_ERRNO(_FUNCNAME) \
    do { \
        int __err = errno; \
        char __msg[1024] = {}; \
        strerror_r(__err, __msg, 1024); \
        WARN("%s: %s", _FUNCNAME, __msg); \
    } while (0)

#define WARN_IF_NEG_WITH_ERRNO(_VAR, _FUNCNAME) \
    do { \
        if ((_VAR) < 0) { \
            WARN_WITH_ERRNO(_VAR, _FUNCNAME) \
        } \
    } while (0)


#define PANIC_WITH_ERRNO(_FUNCNAME) \
    do { \
        int __err = errno; \
        char __msg[1024] = {}; \
        strerror_r(__err, __msg, 1024); \
        PANIC("%s: (%d) %s", _FUNCNAME, __err, __msg); \
    } while (0)


#define PANIC_IF_NEG_WITH_ERRNO(_VAR, _FUNCNAME) \
    do { \
        if ((_VAR) < 0) { \
            PANIC_WITH_ERRNO(_FUNCNAME); \
        } \
    } while (0)


#define PANIC_IF_NULL_WITH_ERRNO(_VAR, _FUNCNAME) \
    do { \
        if ((_VAR) == NULL) { \
            PANIC_WITH_ERRNO(_FUNCNAME); \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif
