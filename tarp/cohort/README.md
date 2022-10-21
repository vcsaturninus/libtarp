# Cohort - simple C test runner

_Updated May 21 2022_

Very simple test runner written in C. Ideal for calling a number of
minimal tests in a Makefile-based compilation process as for validating
a data structure implementation.

This is an unsophisticated test runner without any bells and whistles.

The user:
 * initializes the test list
 * adds tests to the test list
 * calls the test runner to run the tests in the list
 * frees (destroys) the test list at the end

One nice feature is that tests that generate a fatal signal
such as SIGBUS or SIGSEGV do _not_ end the process. Instead, they
are caught and the test runner reports the respective test as _failed_
and then continues on with the rest.

## Usage

Run `make` to run the example shown below.
**NOTE** that in the example shown 3 tests get run:
 - the first one generates a segfault
 - the second one simply return failure
 - the third one actually succeeds

This is intentional and is used to give an idea of the output.

```C
#include <stdlib.h>

#include "cohort.h"

extern enum status example1(void);
extern enum status example2(void);
extern enum status example3(void);

int main(int argc, char **argv){
    struct cohort *testlist = Cohort_init();

    Cohort_add(testlist, example1, "first_example");
    Cohort_add(testlist, example2, "nutty_example");
    Cohort_add(testlist, example3, "third_and_last");

    enum status status = Cohort_decimate(testlist);
    Cohort_destroy(testlist);

    exit(status);
}
```

## Example Output

(Run `make` in the cwd to run this example):
```
└─$ make
Building example ...
Total number of tests: 3
	! test 1    of 3    !FAILED! | first_example                  |		[* caught deadly signal *]
	! test 2    of 3    !FAILED! | nutty_example                  |
	  test 3    of 3    passed.  | third_and_last                 |

	| DONE --> Passed: 1 of 3

make: *** [Makefile:16: run_example] Error 1
```


## License

This software is BSD-2 licensed.


