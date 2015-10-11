Lookup benchmark
==========================
Lookup function benchmark in single thread.

How to run lookup benchmark
==========================
- Prepare 'unity' unit test framework, ruby and gcovr.
- ./configure --enable-developer && make at lagopus top directory.
- cd test/dataplane/benchmark && make depend && make
- Run make check or make check-nocolor or ./benchmark_test
- Run with 2> /dev/null if syslog output is not needed.

Test cases
==========================
So far, test cases are written in benchmark_test.c.
