#pragma once

#include <stdlib.h>


/* ****************************************************************************
    ===========================================================================
    --------------------- Cohort - simple C test runner -----------------------
    ===========================================================================

   This is a casual testing module and functionality is deliberately left basic.
   Having a full-blown test framework that is complicated to use is often a
   deterrent from testing one's code. This module aims to be convenient and
   extremely simple to use, without any bells and whistles. The best way to use
   this is to put the tests in a file and compile and run it when building
   the program e.g. with make by way of sanity-testing it.


   Features and functionality
   ---------------------------

   In a nutshell, the module offers a test runner that will iterate over a linked
   list and execute each test registered there. A per-test as well as an overall
   report is printed to stdout summarizing the tests that ran and which ones failed
   /passed.

   * Each test should be in its own self-contained function. The function should return
     an e_status enum, with possible members being SUCCESS or FAILURE, and take no
     arguments.
   * Each test must be registered with the test runner list. At the end, the test runner
     gets called on the list to execute each registered test in turn.

   Usage
   -----------------------
   * Initialize a 'list' with Cohort_init().
   * Add pointers to test functions to be called to the test list using `Cohort_add()`.
   * Execute all the tests in the list with `Cohort_decimate()`.
   * All the function tests that are to be registered to be called
     should return 0 (enum testStatus SUCCESS) on success and 1 (enum testStatus FAILURE)
     on failure.
   * call Cohort_destroy() on the handle returned by Cohort_init().

   License
   ----------------------

   BSD 2-Clause License

   Copyright (c) 2022, vcsaturninus <vcsaturninus@protonmail.com>
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   ============================================================================ /
   *****************************************************************************/

/*
 * Each test function should return an enum testStatus */
enum testStatus {
    TEST_PASS=0,
    TEST_FAIL=1
};

/*
 * Each test function must have this prototype:
 *  - take no arguments
 *  - return an enum testStatus
 */
typedef enum testStatus (*testfunc)();

/*
 * Encapsulation for a test object.
 * Each test function is associated with a struct test
 * when inserted into the testlist.
 */
struct test{
    testfunc testf_ptr;      // test to be executed
    char *test_name;      // the name of the test
    struct test *next;    // next test in the testlist
};

/*
 * Test list. Cohort_decimate() will iterate over this list
 * and execute each test in turn.
 */
struct cohort{
    struct test *head;
    struct test *tail;
    size_t count;
};

/* |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */

/*
 * Initialize testlist.
 */
struct cohort *Cohort_init(void);

/*
 * Append test to the testlist to be called by the test runner.
 */
void Cohort_add(struct cohort* testlist, testfunc f, char test_name[]);

/*
 * Return count of tests in the test list
 */
size_t Cohort_count(struct cohort *testlist);

/*
 * Run all tests in the testlist (cohort) and report the results.
 * Return an enum testStatus -- either SUCCESS or FAILURE.
 */
enum testStatus Cohort_decimate(struct cohort *testlist);

/*
 * Free the memory associated with each test in the testlist,
 * and then finally the testlist itself.
 */
void Cohort_destroy(struct cohort *testlist);

//
/************************************************************
 * Macro helpers that may be used if the cohort test runner |
 * is not.                                                  |
 * See tests/bitarray/tests.c for an example of how these   |
 * are used.                                                |
 ***********************************************************/

/*
 * Environment variables:
 * 1) TEST_DEBUG=[ALL|FAILED]:  enable the cond_test_debug_print prints in the
 *    test files; enable all of them (=ALL), or only ones guarded by TEST_FAIL
 *    conditions (=FAILED).
 * 2) STOP_ON_FAIL: stop as soon as one test fails. Do not continue past that.
 */

#define NUM_TESTS_RUN      num_run
#define NUM_TESTS_PASSED   num_passed
#define TEST_PASSED        passed

#define prepare_test_variables() \
    int NUM_TESTS_RUN = 0; \
    int NUM_TESTS_PASSED = 0; \
    int TEST_PASSED = 0;

#define update_test_counter(passed, f)  \
    ++NUM_TESTS_RUN;  \
    if (passed) ++NUM_TESTS_PASSED; \
    printf("[ ] test %4i %9s | %s()%s\n", \
            num_run, \
            (passed) ? "Passed" : "FAILED !!", \
            tkn2str(f), \
            (passed) ? "" : " ~ called at line " tkn2str(__LINE__)\
            ); \
    if (getenv("STOP_ON_FAIL") && !passed) exit(1)

/*
 * test runner; forward variadic arguments to any function,
 * then update tests state.
 * */
#ifndef __cplusplus

#define run(f, expected, ...) \
    TEST_PASSED = (f(__VA_ARGS__) == expected); \
    update_test_counter(TEST_PASSED, f);

#else   /* __cplusplus */

#include <functional>
/* user must call update_test_counter manually with the result */
template <typename Func, typename ...Params>
bool run(Func f, enum testStatus expected, Params&&... params)
{
    return ( f(std::forward<Params>(params)...) == expected);
}
#endif

#define report_test_summary() \
    printf("\n Passed: %i / %i\n", NUM_TESTS_PASSED, NUM_TESTS_RUN); \
    if (NUM_TESTS_PASSED != NUM_TESTS_RUN) exit(1)

#define cond_test_debug_print(status, ...) \
    do { \
        const char *cond__ = getenv("TEST_DEBUG"); \
        if (cond__ && match(cond__, "ALL")){ \
            info(__VA_ARGS__); \
        } else if (cond__ && match(cond__, "FAILED") && status==TEST_FAIL){ \
            info(__VA_ARGS__); \
        } \
    } while (0)


