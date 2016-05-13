<!-- -*- mode: markdown -*- -->

How to genarate benchmark test
==============================

Introduction
------------
This is the file that describes how to generate the benchmark tests.
You must write a function of the benchmark target in advance.

Features of tools
----------------------
1. Genarate benchmark tests from parameter file(YAML).
2. Measure the throughput(min, max, avg), nsec/packet(min, max, avg).
3. Specify the number of measuring times, and batch size.
4. Specify the packet capture file.

TODO
----
1. Measure the cache miss, etc. (useing PAPI)
2. Read the datastore DSLs. (State lagopus is started)

Samples
-------
There is a sample in the following directory.

`<LAGOPUS>/tools/benchmark/sample/`

* benchmark parameter file
  `<LAGOPUS>/tools/benchmark/sample/benchmark/benchmark_sample.yml`
* pcap file
  `<LAGOPUS>/tools/benchmark/sample/benchmark/icmp.pcap`
* That contains the function of benchmark target in the following files
  `<LAGOPUS>/tools/benchmark/sample/dump_pkts/dump_pkts.[ch]`

How to genarate benchmark
---------------------------
## Recommended
* Ubuntu 16.04

## Related packages
 * Python 2.7.11 or Python 3.5.1
 * pip
 * PyYAML 3.11
 * Jinja2 2.7.3
 * six 1.9.0

## 1. Install the packages
You execute the following command:

```
 % sudo apt-get install python python-dev python-pip
 % sudo pip install --requirement tools/benchmark/requirements.pip
```

## 2. Execute configure & make for lagopus
You execute `configure` script with --enable-developer,--enable-dpdk options.
And you execute make.

```
 % ./configure --enable-developer
 % make
```

## 3. Create a benchmark parameter file (YAML)
You write the following fields in the file in YAML format.

### fields
| top level field | 2nd level field | vlue type | required | description |
|--|--|--|--|--|
| benchmarks || list | O ||
|| file | string | O | This field describes the name of benchmark file. |
|| include\_files | list of string | O | This field describes the name of include files (\*.h, \*.c). |
|| lib\_files | list of string | X | This field describes the name of lib files(\*.o, \*.so, etc.). |
|| external\_libs | string | X | This field describes the external libs(e.g. "-lFOO -lBAR"). |
|| setup\_func | string | X | This field describes the name of setup function [void func(void)]. |
|| teardown\_func | string | X | This field describes the name of teardown function [void func(void)]. |
|| target\_func | string | X | This field describes the name of benchmark target function [lagopus\_result\_t func(void *pkts, size\_t pkts\_size)]. |
|| pcap | string | O | This field describes the name of pcap file (\*.pcap, etc.). |
|| times | int | X | This field describes the number of executions of target\_func (default: 1).|
|| batch_size | int | O | This field describes the size of batches(default: 0) . 0 is read all packets in pcap file. |

e.g.)
sample: tools/benchmark/sample/benchmark/benchmark\_sample.yml

```
benchmarks:
  - file : benchmark_sample1
    include_files:
      - ../dump_pkts/dump_pkts.h
    lib_files:
      - ../dump_pkts/dump_pkts.o
    setup_func: setup
        teardown_func: teardown
    target_func: dump_pkts
    pcap: icmp.pcap
  - file : benchmark_sample2
    include_files:
      - ../dump_pkts/dump_pkts.c
    target_func: dump_pkts
    pcap: icmp.pcap
```

## 4. Genarate benchmark files
This tool (`benchmark_generator.py`) generates Makefile,
Makefile.in, *.c, *.h files from `<BENCHMARK_PARAMETER_FILE>`.

You execute the following command:
```
 % ./tools/benchmark/benchmark_generator.py <BENCHMARK_PARAMETER_FILE> <OUT_DIR>
```

e.g.)
sample:

```
 % cd tools/benchmark/sample/benchmark
 % ../../benchmark_generator.py benchmark_sample.yml ./
 % ls
   Makefile  Makefile.in  benchmark_sample.yml
   benchmark_sample1.c  benchmark_sample2.c  icmp.pcap
```

## 5. Compile & Run benchmark
You execute the following command:

* Compile:
```
 % cd <OUT_DIR>
 % make
```

* Run benchmark:
```
 % make benchmark
   or
 % ./<BENCHMARK_EXECUTABLE_FILE> [-t <MEASURING_TIMES>] [-b <BATCH_SIZE>] <PCAP_FILE>
   positional arguments:
     PCAP_FILE           Packet capture file.

   optional arguments:
     -t MEASURING_TIMES  Number of executions of benchmark target func(default: 1).
     -b BATCH_SIZE       Size of batches(default: 0).
                         0 is read all packets in pcap file.
```

e.g.)
sample:

```
 % make benchmark
   or
 % ./benchmark_sample1 icmp.pcap
```
