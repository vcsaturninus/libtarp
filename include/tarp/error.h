#ifndef TARP_ERROR_H
#define TARP_ERROR_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <tarp/log.h>


/*
 * Constants describing error conditions, meant to either be returned as
 * error codes or serve as the basis for an "exception" being thrown.
 *
 * Of course, C does not have exceptions so in that case the error code points
 * to an error condition that is either unrecoverable or for which recovery is
 * undesired/able (e.g. breach of function preconditions). In this case, the
 * program will simply exit with an error message. Essentially, these serve as
 * assertions that don't disappear in non-debug code.
 * ~~ EXAMPLE:
 * If memory allocation fails, the program should just exit.
 * By decorating the calls to the memory allocator we can ensure termination on
 * allocation failure and not have to check for 'NULL' all over the place. It's
 * worth noting that typical systems e.g. Linux will invoke the OOM-killer and
 * likely terminate the process anyway should the system be out of memory.
 */
enum ErrorCode{
    ERRORCODE_SUCCESS = 0, /* success */
    ERROR_BADALLOC=1,
    ERROR_RUNTIMEERROR,
    ERROR_INTERNALERROR,
    ERROR_NOSPACE,
    ERROR_OUTOFBOUNDS,
    ERROR_BADPOINTER,
    ERROR_INVALIDVALUE, /* generic, encompasses ERROR_OUTOFBOUNDS etc */
    ERROR_EXPIRED,
    ERROR_EMPTY,
    ERROR_BADTIMEPOINT,
    ERROR_MISCONFIGURED,
    ERROR_IO,
    ERROR_READ,
    ERROR_WRITE,
    ERROR_CONFLICT,
    ERRORCODE_LAST__
};

/*
 * return a statically allocated string describing the exception or error,
 * associated with CODE. */
const char *tarp_strerror(unsigned int code);


void throw__(
        enum ErrorCode code,
        const char *file,
        const char *func,
        int line,
        const char *condition,
        const char *extra);

/*
 * Throw an exception; here 'cond' is *not* checked, only printed */
#define THROW__(cond, exception) \
    throw__(exception, __FILE__, __func__, __LINE__, #cond, NULL)

/*
 * Like THROW, but with an arbitrary number of extra arguments as for printf */
#define THROWS__(cond, exception, ...)                                  \
    do {                                                                \
        char buff[1025] = {0};                                          \
        snprintf(buff, 1025, __VA_ARGS__);                              \
        throw__(exception, __FILE__, __func__, __LINE__, "", buff);     \
    } while(0)


#define THROW(exception)       THROW__("", exception)
#define THROWS(exception, ...) THROWS__("", exception, __VA_ARGS__)

/*
 * Throw an exception if condition is true */
#define THROW_ON(cond, exception)             \
    do {                                      \
        if (cond) THROW__(cond, exception);   \
    } while(0)

/*
 * Like THROW_ON, but with an arbitrary number of
 * extra arguments as for printf */
#define THROWS_ON(cond, exception, ...)                        \
    do {                                                       \
        if (cond) THROWS__(cond, exception, __VA_ARGS__);      \
    } while(0)


/*
 * Print warning if condition is true */
#define WARN_ON(cond, ...) do{ if(cond) warn(__VA_ARGS__); } while(0)


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
