#ifndef __TARP_COMMON_H__
#define __TARP_COMMON_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


void _dbglog_(int line, const char *file, const char *func, char *fmt, ...);

/*
 * Get length of array (note: NOT array decayed to pointer!)
 */
#define ARRLEN(arr)            (sizeof(arr) / sizeof(arr[0]))

#define membersz(type, member) sizeof(((type *)0)->member)

/*
 * The maximum value of the specified *unsigned* type. This is when all bits
 * are 1. NOTE - 2's complement is assumed */
#define typeumax(type) (~((type)0))

/*
 * Stringify token x to allow concatenation with string literals */
#define tostring__(x) #x
#define tkn2str(x)    tostring__(x)

/*
 * Suppress warning about variable being declared but unused.
 *
 * --> x
 *     Variable to suppress the warning for; can be preceded by any
 *     number of qualifiers e.g. UNUSED(static const int myvar) = 13;
 *
 *  Casting to void will always work; using the unused attribute works
 *  with gcc and clang.
 */
#define UNUSED(x) \
  do {            \
    (void)(x);    \
  } while (0)

/*
 * Useful for namespacing enums i.e. prefixing all enum members
 * with PREFIX. Meant to be wrapped in another macro that passes
 * a hardcoded PREFIX value implicitly, e.g.
 * #define PREFIX GROND
 * #define enum_ns(member) enum_namespace(PREFIX, member)
 * such that the calls actually being used in code are enum_ns(member).
 */
#define _do_add_enum_member_prefix(PREFIX, member) PREFIX##_##member
#define enum_namespace(PREFIX, member) \
  _do_add_enum_member_prefix(PREFIX, member)

/*
 * Safe allocation: exits the program if calloc/malloc/realloc return NULL;
 *
 * Calls calloc (size>0, ptr=NULL), realloc (size>0, ptr!=NUL), or free (size=0,
 * ptr != NULL).
 *  - Calloc is called instead of malloc to ensure the data is zeroed.
 *
 */
void *salloc(size_t size, void *ptr);

// print n newlines.
#define nl(n)                             \
  do {                                    \
    for (int i = n; i > 0; --i) puts(""); \
  } while (0);

#define match(a, b)     (strcmp(a, b) == 0)
#define matchn(a, b, n) (strncmp(a, b, n) == 0)
#define bool2str(val)   ((val) ? "True" : "False")

/*
 * Prototype for a function that when called with two non-NULL
 * void pointers a and b, it first casts them to their proper types,
 * compares them, and returns one of the following values:
 *    -1  (a<b)
 *     1  (a>b)
 *     0  (a==b)
 *
 * IOW it returns one of the constant members of the comparatorResult
 * enumeration. */
enum comparatorResult { LT = -1, EQ = 0, GT = 1 };
typedef enum comparatorResult (*comparator)(const void *a, const void *b);

// a triple that makes it convenient to build and print
// informative error strings using perr() (see below).
// ok: whether the function was successful or not.
// e: an error string. Should only be looked at if !ok.
// errnum: the value of errno. Should only be looked at if !ok and
// should only be considered if != 0. Otherwise it should be ignored.
struct result {
    bool ok;
    const char *e;
    int errnum;
};

// clang-format off
#ifdef __cplusplus
#define RESULT(ok, errstr, errnum) result{ok, errstr, errnum}
#else
#define RESULT(ok, errstr, errnum)  (struct result) { ok, errstr, errnum }
#endif
// clang-format on

// Print an error string based on the struct result input.
// Thread safe.
void perr(const struct result res);

#ifndef __cplusplus
/*
 * Predicate helpers that evaluate to boolean true when the result of
 * comparing a and b using the given comparator is LT, GT, or EQ. */
#define lt(a, b, cmp)  (cmp(a, b) < 0)
#define gt(a, b, cmp)  (cmp(a, b) > 0)
#define eq(a, b, cmp)  (cmp(a, b) == 0)
#define lte(a, b, cmp) (cmp(a, b) <= 0)
#define gte(a, b, cmp) (cmp(a, b) >= 0)

#endif /* ifndef __cplusplus */

/*
 * Given a start pointer 'base' to type TYPE, get a pointer OFFSET positions
 * from it . */
#define get_pointer_at_position(base, type, offset) (((type *)base) + offset)

/*
 * Macro that expands to 'static' when called with argument type=private,
 * and to nothing when called with argument type=public.
 * Useful in macros that autogenerate code so the visibility can be
 * controlled via an argument to the macro.
 *
 * E.g.
 * vis(public)  int i;     // int i
 * vis(private) int i;     // static int i
 */
#define vis_private static
#define vis_public
#define vis(type) vis_##type

#ifdef __cplusplus
} /* extern "C" */
#endif


#ifdef __cplusplus
#include <cstring>
#include <string>
inline std::string geterr(const struct result res) {
    if (res.ok) {
        return "ok";
    }

    std::string err = res.e;

    if (res.errnum == 0) {
        return err;
    }

    // thread-safe strerror.
    char buff[256];
    memset(buff, 0, sizeof(buff));

#ifdef _GNU_SOURCE
    // the GNU version of strerror_r rather than the XSI one
    const char *errstr = strerror_r(res.errnum, buff, sizeof(buff));
#else
    strerror_r(res.errnum, buff, sizeof(buff));
    const char *errstr = buff;
#endif

    return err + ": " + errstr;
}
#endif


#endif
