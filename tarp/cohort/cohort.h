#ifndef MOCOHORT_H
#define MOCOHORT_H

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
     should return 0 (enum status SUCCESS) on success and 1 (enum status FAILURE)
     on failure.

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
 * Each test function should return an enum status */
enum status{SUCCESS, FAILURE};

/* 
 * Each test function must have this prototype:
 *  - take no arguments
 *  - return an enum status
 */
typedef enum status (*testf)();

/* 
 * Encapsulation for a test object. 
 * Each test function is associated with a struct test
 * when inserted into the testlist.
 */
struct test{
    testf testf_ptr;      // test to be executed
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
void Cohort_add(struct cohort* testlist, enum status (*testf)(), char test_name[]);

/*
 * Return count of tests in the test list 
 */
size_t Cohort_count(struct cohort *testlist);

/* 
 * Run all tests in the testlist (cohort) and report the results.
 * Return an enum status -- either SUCCESS or FAILURE.
 */
enum status Cohort_decimate(struct cohort *testlist);

/*
 * Free the memory associated with each test in the testlist,
 * and then finally the testlist itself.
 */
void Cohort_destroy(struct cohort *testlist);

#endif
