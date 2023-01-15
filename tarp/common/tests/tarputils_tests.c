#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#include <tarp/timeutils.h>
#include "cohort.h"

enum status can_compare_timespecs(void){
    struct timespec before;
    struct timespec after;
    struct timespec diff;
    memset(&before, 0, sizeof(struct timespec));
    memset(&after, 0, sizeof(struct timespec));
    memset(&diff, 0, sizeof(struct timespec));
    assert(clock_gettime(CLOCK_MONOTONIC, &before) == 0);
    mssleep(2500, true);
    assert(clock_gettime(CLOCK_MONOTONIC, &after) == 0);
    timespec_diff(&after, &before, &diff);
    
    char before_str[1024] = {0};
    char after_str[1024] = {0};
    timespec_tostring(&before, "before", before_str, 1024);
    timespec_tostring(&after, "after", after_str, 1024);
    fprintf(stderr, "comparing %s to %s\n", after_str, before_str);
    if (timespec_cmp(&after, &before) != 1) return FAILURE;
    return SUCCESS;
}

