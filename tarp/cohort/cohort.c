#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <setjmp.h>

#include "cohort.h"


// declarations
static void register_sighandler(void);


/* -------------------------------- 
   global variables (file-scoped)
   ===============================*/

// flag set by signal handler; tested and cleared by test runner (Cohort_decimate()) 
// to know if it's a fresh start or resumption after signal interrupt
static volatile sig_atomic_t sig_delivered = 0;    

// num of tests run so far; if fresh start, 0, else used to 
// figure out where to resume in the testlist after signal was caught (sigsegv etc);
// i.e. start at test n+1 in the testlist if num_run is n.
static size_t num_run = 0;
size_t testcount = 0;

// Flag to mark that the sig_env variable has been initialized, ie its safe to do a long jump
static bool safe_to_jump = false;  

// environment state for the signal handler
static sigjmp_buf sig_env;

// global test report buffer updated for each test by the test runner;
static struct{
    size_t       test_number;
    bool         passed;
    bool         got_fatal_signal;
    char        *test_name;
    struct test *test_ptr;  // resume at test_ptr->next after signal interrupt
}test_report;


////////////////////////////////////////////////////////////////////////////

/* -------------------------------- 
   function definitions
   ===============================*/

/* 
 * Print out the global test_report struct.
 *
 * This function should be called after each test in order to print out 
 * its respective report. Therefore it should be called by the test runner
 * (Cohort_decimate()) either as it iterates through the test list, or after 
 * recovery from signal interrupt.
*/
static void print_test_report(void){
    if (test_report.passed == true){
        printf("\t  test %-4zu of %-4zu passed.  | %-30s | \n", 
                test_report.test_number, 
                testcount, 
                test_report.test_name);
    }else{
        if (test_report.got_fatal_signal){
            printf("\t! test %-4zu of %-4zu !FAILED! | %-30s |\t\t[* caught deadly signal *]\n", 
                    test_report.test_number, 
                    testcount, 
                    test_report.test_name);
        }
        else{
            printf("\t! test %-4zu of %-4zu !FAILED! | %-30s |\n", 
                    test_report.test_number, 
                    testcount, 
                    test_report.test_name);
        }
    }
}

/* 
 * Populate the global test_report struct.
 *
 * This function should be called after each test to save its respective report
 * details. This is ahead of calling print_test_report().
 *
 * It should therefore be called by the test runner either as it iterates over the 
 * test list or after recovery from signal interrupt.
*/
static void save_test_report(bool passed, bool got_fatal_signal, size_t test_number, struct test *test_ptr){
    test_report.test_ptr = test_ptr;
    test_report.test_name = test_report.test_ptr->test_name;
    test_report.test_number = test_number;
    test_report.got_fatal_signal = got_fatal_signal;
    test_report.passed = passed;
}

/*
 * Allocate heap space for a new test struct.
 */
static struct test *__Cohort_newtest(void){
    struct test *new = calloc(1, sizeof(struct test));
    assert(new && "failed to allocate heap space for new struct test");

    new->test_name = NULL;
    new->testf_ptr = NULL;
    new->next = NULL;

    return new;
}

/*
 * Allocate heap space for new test list and initialize it.
 */
struct cohort *Cohort_init(void){
    struct cohort *new = calloc(1, sizeof(struct cohort));
    assert(new && "Failed to mallocate new test list");

    new->count = 0;
    new->head = NULL;
    new->tail = NULL;
    
    // register signal handler
    register_sighandler();

    // disable buffering in stdio lib functions
    setbuf(stdout, NULL);

    return new;
}

/*
 * Release all heap memory associated with the test list 
 * (i.e. the test list itself and each test struct in it).
 */
void Cohort_destroy(struct cohort *testlist){
    struct test *temp = NULL;    
    struct test *current = NULL;    
    current = testlist->head;

    while(current){
        temp = current;
        current = current->next;
        free(temp);
    }
    free(testlist);
}

/* Append test to test list */
void Cohort_add(struct cohort *testlist, enum status (*testf)(void), char test_name[]){

    assert(testlist);
    assert(testf);
    assert(test_name);

    struct test *new = __Cohort_newtest();
    new->testf_ptr = testf;
    new->test_name = test_name;

    // empty list
    if (!(testlist->head)){
        testlist->head = new;
        testlist->tail = new;
        testlist->count++;
    }
    // not empty, append after current tail
    else{
        testlist->tail->next = new;
        testlist->tail = new;
        testlist->count++;
    }
}

/*
 * Return count of tests in the test list.
 */
size_t Cohort_count(struct cohort *testlist){
    return testlist->count;
}

/*
 * Return pointer to nth test in the test list.
 * This is used when resuming after receipt of signal.
 */
static struct test *Cohort_get_nth(struct cohort *testlist , size_t n){
    if (n > Cohort_count(testlist)){
        return NULL;  // list out of tests
    }

    size_t i = 1;
    struct test *current = testlist->head;  // start at node number 1

    while (i<n && current != NULL){
        current = current->next;
        i++;
    };

    return current;
}

/* 
 * Handle sigsegv, sigbus etc signals.
 */
static void sighandler(int signum){
    // if safe_to_jump isn't true, it means the environment hasn't been initialized
    // (which would cause the handler to fail as well), so simply exit;
    if (!safe_to_jump){
        // stdio calls are NOT async-signal safe; but we're exitting anyway
        puts("[ ] sigjmp environment not initialized!");
        exit(18);
    }

    // else, the env has been initialized, so it's safe to perform a nonlocal goto

    // let test runner know this is a post-signal-receipt resumption()
    sig_delivered = 1;
    siglongjmp(sig_env, 1);    // the 2nd arg is useful if you want to distinguish between calls to siglongjmp
                               // when you perform jumps to the same point from multiple
                               // places in the program; in this particular case, there's
                               // only one, so it doesn't matter;
}

/*
 * Register signal handler and set signal disposition.
 */
void register_sighandler(void){
    struct sigaction sigact;
    sigact.sa_handler = sighandler; 
    sigemptyset(&sigact.sa_mask); // do not block any signals
    sigact.sa_flags=0;
     
    /* The .sa_mask can be used to add signals to the mask to be blocked during execution
     * of the signal handler; I'm not adding anything, so it can be left empty; 
     * by default, the signal(s) that the signal handler is regstered to handle is added by
     * default. All the signals in sa_mask are added to the process's signal mask
     * automatically before the signal handler executes and removed automatically once 
     * the signal handler finishes executing. This is so that the signal handler is not
     * interrupted when in the middle of handling a received signal.
     */

    // don't save old sig action configuration
    if (sigaction(SIGSEGV, &sigact, NULL) == -1){
        puts("[ ] Failed to register signal handler for SIGSEGV");
    }
    if (sigaction(SIGBUS, &sigact, NULL) == -1){
        puts("[ ] Failed to register signal handler for SIGBUS");
    }
}

/* 
 * Iterate over the test list and run each test in turn.
 *
 * For each test, a test report is printed; an overall report
 * is printed at the end as well detailing which and the number
 * of tests that passed/failed.
 *
 * Return SUCCESS if all tests passed, else FAILURE. The caller should
 * propagate this status code to exit() such that if the test runner
 * failed, the process itself should also exit with an error code.
 *
 *
 * Notes about async signal safety
 * --------------------------------
 *
 * Calling stdio functions from a signal handler is unafe as they are _not_
 * async-signal safe/reentrant. Calling them after making a non-local
 * jump with siglongjmp() is exactly the same: they are _not_ safe.
 * However, we're relying here on this line:
 *          enum status code = current->testf_ptr();
 * being the only possible cause of a fatal signal that would trigger the
 * handler. Adjacent stdio calls made by the test runner before and after
 * should never generate a fatal signal. I.e. they should never be
 * interrupted by the signal handler and be left in an inconsistent state.
 *
 * So if indeed the program only ever crashes on the aforementioned line (that
 * is, if an actual test is the only possible cause of a SIGSEGV or SIGBUS), 
 * then there is nothing to worry about. 
 * However, on the other hand, if that assumption is not true there's either:
 * 1) a bug in the code here (because reasonably no calls to stdio should ever
 * have reason to generate a fatal signal - _in this code here_)
 * 2) a test that got run corrupted memory to the point where it caused a failure
 * in a call to a stdio function. This is possible and in this case it would be
 * unreasonable to even try to recover. The program should abort (although in the
 * current implementation the signal does get caught and the program does attempt
 * to keep going).
 * Do note that a test that gets run could very well corrupt memory by writing out
 * of bounds and _still_ not generate a SIGSEGV signal!
 */
enum status Cohort_decimate(struct cohort *testlist){
    if (!testlist->head){
        printf("[ ] Test list empty : nothing to do.\n");
    }
    assert(Cohort_count(testlist) > 0);

    testcount = Cohort_count(testlist);
    size_t total_passed = 0;
    struct test *test_to_run = NULL;

    printf("Total number of tests: %zu\n", testcount);

    /* (re)initialize sig state variable.
     * siglongjmp makes long jump to here.
     */
    if (sigsetjmp(sig_env, 1) == 0){ 
        // only execute the body here on FIRST init (when sigsetjmp() returns 0)
        safe_to_jump = true;

        /* If starting clean from the beginning (global sig_delivered is false) rather than
         * resuming after signal receipt, then start at the head-end of the test list.
         */
        if (!sig_delivered){ 
            test_to_run = testlist->head;
            assert(test_to_run);
        }
    }
    
    /* Else, this is a resumption after signal interrupt caused by the test at
     * num_run; resume with num_run+1
     */
    if (sig_delivered){
        sig_delivered = 0;  // clear flag

        // print test report for previous (failed) test;
        print_test_report();
        test_to_run = Cohort_get_nth(testlist, num_run+1);
    }
   
    for (struct test *current = test_to_run; current; current = current->next){
        // we increment num_run BEFORE running the test, but only increment 
        // total_passed AFTER. That way, if the test segfaults, we know the last test
        // wasn't successful. If the signal handler runs, it automatically assumes 
        // there's been a crash and a test failed with a memory violation or somesuch 
        // fault and it prints a negative test report.
        bool passed = false;
        ++num_run;
        // have (failed) test report prepared in case test fails with fatal signal
        save_test_report(passed, true, num_run, current);

        enum status code = current->testf_ptr();

        if (code == SUCCESS){
            passed = true;
            total_passed++;
        }
    
        // actually did not crash with signal interrupt: amend test report
        save_test_report(passed, false, num_run, current);
        print_test_report();
    }

    printf("\n\t| DONE --> Passed: %zu of %zu\n\n", total_passed, testcount);

    // at least one test failed
    if (total_passed != testcount){
        return FAILURE;
    }
    return SUCCESS;
}

