<!-- -*- mode: markdown ; coding: us-ascii-dos -*- -->

How To Write Unit Tests In The Lagopus
======================================

Introduction
------------

The lagopus has the test code for unit tests,
and you can run unit tests by only one command; `make check`.

You, the lagopus developer, might frequently add new features to the lagopus.
And you would better write the unit tests for them.

This document describes how to write unit tests and how to execute these tests.

In the first section "Unity Test Framework", we introduce the test framework which the lagopus uses.
The second section "Test your code" describes how to unit test the lagopus.

Enjoy coding!

Unity Test Framework
--------------------

The lagopus uses Unity (https://github.com/ThrowTheSwitch/Unity.git) for unit tests.

Unity is a xUnit like test framework for C.
Unity uses C for providing basic faculties such as assertion macros.
And it uses Ruby for more complex faculties; test discovery, test runner generation and so on.

By the support of Unity, you can write the Four-Phase test case (i.e. Fixture setup, Exercise System, Verify outcome, Fixture teardown) in C.

### Recommended
* Unity v2.1.0 (hash: 731e0f6b5f96b83adb94c209b582765c14215b2c)
* Ruby 1.9.3
* gcovr 2.4
* jq 1.3
* PAPI 5.4.3.0

### Install Ruby

Install ruby (on Debian GNU/Linux or Ubuntu).

```
$ sudo apt-get install ruby
```

### Install gcovr

Get gcovr version 2799 (latest version might not work with Unity).

```
$ curl -O https://software.sandia.gov/trac/fast/export/2799/gcovr/trunk/scripts/gcovr
```

Then move gcovr and add exectable permission to it.

```
$ sudo mv gcovr /usr/local/bin
$ sudo chmod a+x /usr/local/bin/gcovr
```

If Ubuntu version is 13.04 or higher, you can use apt-get.

```
$ sudo apt-get install gcovr
```

### Install jq and PAPI

Install jq (Command-line JSON processor) and PAPI (Performance Application Programming Interface).

```
$ sudo apt-get install jq libpapi-dev
```

Test Your Code
---------------

This section tells a workflow of unit testing:

-  step 1. Create A Directory For Test Code
-  step 2. Write Your Test Code
-  step 3. Create Makefile.in For The Tests
-  step 4. Reconfigure The Building Environment
-  step 5. Run Your Tests!

For further discussion,
presume we are about to write the unit tests for a module named `nicefeature`.
The functionalities of the nicefeature module are provided by a library named 'libnicefeature.la'.
This library is able to be built with a decent Makefile.in that exists in a directory named 'nicefeature', which is identical to the module name itself.

### step 1. Create A Directory For Test Code

First of all, create a directory which will contain new test files.
You can name the directory as you like, but we recommend to name it `test`.

To inform the new directory to the building environment,
add a `DIRS` macro to `Makefile.in` in the `nicefeature` directory as below:

```Makefile
DIRS = test
```

If a `DIRS` macro is already defined,
just append a name of the directory to the list of values.

### step 2. Write Your Test Code

Create a file named `nicefeature_test.c`, in which you write the test code.
It is very important that the base name of the file ends with `_test`,
since the building environment automatically generates test runners for files whose name matches this pattern.

Here's `nicefeature_test.c`.

```c
  1  #include <unity.h>
  2
  3  #include <nicefeature.h>
  4
  5  struct nicefeature * nf;
  6
  7  void
  8  setUp() {
  9    // set up fixture
 10    nf = new_feature();
 11  }
 12
 13  void
 14  tearDown() {
 15    // tear down fixture
 16    free(nf);
 17  }
 18
 19  void
 20  test_NiceFeature() {
 21    int ret;
 22    setValue(nf, 5);
 23    ret = getValue(nf);
 24    TEST_ASSERT_EQUAL(5, ret);
 25  }
 26
 27  void
 28  test_BadFeature() {
 29    int ret;
 30    setValue(nf, -1);
 31    ret = getValue(nf);
 32    TEST_ASSERT_EQUAL(0, ret);
 33  }
 34
 35  void
 36  test_NotImplementedFeature() {
 37    int ret;
 38    struct nicefeature * nf2 = new_feature();
 39    TEST_IGNORE();
 40    setValue(nf, 5);
 41    setValue(nf2, 6);
 42    ret = compare(nf, nf2);
 43    TEST_ASSERT_EQUAL(-1, ret);
 44    free(nf2);
 45  }
```

The test file should include `unity.h` (line 1 in the example).

Write `setUp()` and `tearDown()` (line 8 and 14 in the example).
`setUp()` is called before all tests run, on the other hand `tearDown()` is called after all tests executed.

The function with the prefix `test_` is treated as one test case.
Every test cases executed in the order of its appearance in the file.
In the example, the generated test runner calls `test_NiceFeature()`, `test_BadFeature()`, then `test_NotImplementedFeature()`.

You can use various assertion macros.
Here's examples:

 - `TEST_ASSERT_EQUAL(expected, actual)`.
    This macro compares two arguments as 32 bit integers.
    A first argument is an expectation and a second one is a target to be verified.
    Unity assumes 32 bit integers by default.
    To change the size of integer, define `UNITY_INT_WIDTH`.

 - `TEST_ASSERT_EQUAL_UINT32(expected, actual)`
    Compares arguments as unsigned 32 bit integers.

 - `TEST_ASSERT_NULL(actual)`
    Verifies a value is NULL.
    On the other hands, `TEST_ASSERT_NOT_NULL(actual)` verifies a value is not NULL.

 - `TEST_IGNORE`
    Ignore the lines after this macro.

Each macro has a variation with the suffix `_MESSAGE`.
These macros have an argument which is used as a message emitted when the assertion fails.

For more detail, see `unity.h`.

### step 3. Create Makefile.in For The Tests

Write `nicefeature/test/Makefile.in` for tests as below:

```
 1  TOPDIR      = @TOPDIR@
 2  MKRULESDIR  = @MKRULESDIR@
 3
 4  TESTS = nicefeature_test
 5  SRCS = nicefeature_test.c
 6  TEST_DEPS =  $(TOPDIR)/nicefeature/libnicefeature.la
 7
 8  include $(MKRULESDIR)/vars.mk
 9  include $(MKRULESDIR)/rules.mk
10  include .depend
```

From line 1 to 2 and from 8 to 10 are the Makefile boilerplate in the lagopus.

At line 4, the `TESTS` macro informs the building environment that this directory has one or more test codes.
As mentioned in the former section, the test runners are generated from source files whose base name ends with `_test`.
So specify the base name of the source files that contain test codes in `TESTS` macro.
The `TESTS` macro may consist of multiple test runners, and they are generated per source files.

The `nicefeature_test` will be linked to `libexampble.la`,
so `TEST_DEPS` is set to `libnicefeature.la` at line 6 in the example.

### step 4. Reconfigure The Building Environment

After you have finished to write the `Makefile.in`,
tell the building environment that there is a new Makefile to be configured.
To do it, move to the lagopus top directory and edit configure.ac like:

```
AC_CONFIG_FILES(
    ...
    nicefeature/test/Makefile
)
```

Then generate `configure` as:

```
$ autoconf
```

Finally, reconfigure the building environment by issuing:

```
$ ./configure --enable-developer && make
```

Now you can run your tests.

### step 5. Run Your Tests!

Go back to the test directory and run tests as below:

```
$ make check
```

Here's the test result of the example test code.

```
test/test_nicefeature.c:20:test_NiceFeature:PASS
test/test_nicefeature.c:32:test_BadFeature:FAIL: Expected 0 Was -1
test/test_nicefeature.c:39:test_NotImplementedFeature:IGNORE
```

`PASS` means there are no assertion failures in `test_NiceFeature()`,
and `FAIL` informs there is an assertion failure in `test_BadFeature()`.
`IGNORE` means lines after line 39 in `test_NotImplementedFeature()` are ignored.

### Summary

In the example for testing of the example module,
you added files:

 - `nicefeature/test/Makefile.in`
 - `nicefeature/test/nicefeature_test.c`

you modified:

 - `nicefeature/Makefile.in`
 - `configure.ac`

and you reconfigured the environment in the lagopus top directory:

```
$ autoconf
$ ./configure --enable-developer && make
```

and run the tests in the test directory as below:

```
$ make check
```

Reference
---------

 - http://throwtheswitch.org/ ThrowTheSwitch.org, the official home page for Unity and the other frameworks
