<!-- -*- mode: markdown -*- -->

How to generate benchmark test
==============================

Introduction
------------
This is the file that describes how to generate the benchmark tests.
You must write a function of the benchmark target in advance.

Features of tools
----------------------
1. Generate benchmark tests from parameter file(YAML).
2. Measure throughput(min, max, avg), nsec/packet(min, max, avg).
3. Measure cache references/misses, CPU cycles and page faults,
   if `num_measurements: 1`(`-n 1`).
4. Specify the number of measuring times, and batch size.
5. Specify the packet capture file.

TODO
----
1. Measure the cache miss, etc. (using PAPI)
2. Incorporate pipelines.

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

How to generate benchmark
---------------------------
## Recommended
* Ubuntu 16.04

## Related packages
 * PAPI 5.4.3.0
 * Python 2.7.11 or Python 3.5.1
 * pip
 * PyYAML 3.11
 * Jinja2 2.7.3
 * six 1.9.0

## 1. Install the packages
You execute the following command:

```
 % sudo apt-get install python python-dev python-pip libpapi-dev
 % sudo pip install --requirement tools/benchmark/requirements.pip
```

## 2. Execute configure & make for lagopus
You execute `configure` script with --enable-developer,--enable-dpdk options.
And you execute make.

```
 % ./configure --enable-developer
 % make
```

## 3. Setup for DPDK
cf.) QUICKSTART.md

* The kernel modules.
* Hugepages.

## 4. Create a benchmark parameter file (YAML)
You write the following fields in the file in YAML format.

### fields
| top level field | 2nd level field | vlue type | required | description |
|--|--|--|--|--|
| benchmarks || list | O ||
|| file | string | O | This field describes the name of benchmark file. |
|| include\_files | list of string | O | This field describes the name of include files (\*.h, \*.c). |
|| lib\_files | list of string | X | This field describes the name of lib files(\*.o, etc.). |
|| external\_libs | string | X | This field describes the external libs(e.g. "-lFOO -lBAR"). |
|| setup\_func | string | X | This field describes the name of setup function [lagopus\_result\_t setup(void *pkts, size\_t pkts\_size)]. |
|| teardown\_func | string | X | This field describes the name of teardown function [lagopus\_result\_t teardown(void *pkts, size\_t pkts\_size)]. |
|| target\_func | string | X | This field describes the name of benchmark target function [lagopus\_result\_t func(void *pkts, size\_t pkts\_size)]. |
|| pcap | string | O | This field describes the name of pcap file (\*.pcap, etc.). |
|| dsl | string | X | This field describes the name of lagopus DSL file. The lagopus module thread is started when you specify this field. |
|| dpdk_opts | string | X | This field describes DPDK opts (default: -cf -n4). |
|| dp_opts | string | X | This field describes Dataplne opts (default: -p3). |
|| num_measurements | int | X | This field describes the number of executions of target\_func (default: 1). 1 is outputs cache references/misses, CPU cycles and page faults.|
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

## 5. Generate benchmark files
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

## 6. Compile & Run benchmark
You execute the following command:

* Compile:
```
 % cd <OUT_DIR>
 % make
```

* Run benchmark:
```
 % sudo make benchmark
   or
 % sudo ./<BENCHMARK_EXECUTABLE_FILE> [-n <NUM_MEASUREMENTS>] [-b <BATCH_SIZE>] <PCAP_FILE>
   positional arguments:
     PCAP_FILE               Packet capture file.

   optional arguments:
     -n NUM_MEASUREMENTS     Number of executions of benchmark target func(default: 1).
     -b BATCH_SIZE           Size of batches(default: 0).
                             0 is read all packets in pcap file.
```

e.g.)
sample:

```
 % sudo make benchmark
   or
 % sudo ./benchmark_sample1 icmp.pcap
```
