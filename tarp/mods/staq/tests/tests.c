#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <tarp/common.h>

#include "cohort.h"

extern enum status keeps_count(void);
extern enum status can_join_stacks(void);
extern enum status can_join_queues(void);

int main(int argc, char **argv){
    UNUSED(argv);
    UNUSED(argc);

    struct cohort *tests = Cohort_init();
    assert(tests != NULL);
   
    Cohort_add(tests, keeps_count, "keeps_count");
    Cohort_add(tests, can_join_stacks, "can_join_stacks");
    Cohort_add(tests, can_join_queues, "can_join_queues");
    enum status res = Cohort_decimate(tests);
    
    Cohort_destroy(tests);
    return res;
}
