<!-- -*- mode: markdown -*- -->

How To Run "log run tests"
======================================
## Recommended
* Ubuntu 14.04

## Related packages
 * Python 2.7.6 or Python 3.4.3
 * pip
 * PyYAML 3.11
 * pykwalify 1.0.1
 * Jinja2 2.7.3
 * ryu 3.19
 * six 1.9.0
 * greenlet 0.4.5    (for ryu)
 * stevedore 1.3.0   (for ryu)

How to run test
---------------------------
## 1. Install packages.

```
 % sudo apt-get install python python-dev python-pip iproute
 % sudo pip install --requirement ./requirements.pip
```

## 2. Install lagopus.
 cf.) {LAGOPUS\_DIR}/INSTALL.txt

## 3. Edit `conf/ds_test.ini` file.

```
 % vi conf/ds_test.ini
```

## 4. Run YAML schema check (for sample).

```
 % ./ds_test.py -C schema/schema.yml sample/
```

## 5. Run test (for sample).
* Usage:
  * ds_test.py [options] TEST_SCENARIOS_FILE_OR_DIR

  * Options:
    * -h, --help      show this help message and exit
    * -c CONFIG       config file (default: ./conf/ds_test.ini)
    * -d OUT_DIR      out dir (default: ./out)
    * -l LOG          log file (default: stdout)
    * -L LOG_LEVEL    log level (debug/info/warning/error/critical, default: info)
    * -C YAML_SCHEMA  YAML schema check

* example:

```
 % sudo ./ds_test.py -L debug -l ./ds_test.log sample/
  out dir: ./out/20150331163451/test
  scenario: texst
  mode: sync
        controller create-destroy                 OK
  out dir: ./out/20150331163451/test_async
  scenario: test_async
  mode: async
        channel create-destroy (10000 times)      OK

  ==============
  ALL(2)/OK(2)/ERROR(0)/UNKNOWN(0)

 % ls -R  out/20150331163451
  out/20150331163451:
  test  test_async

  out/20150331163451/test:
  lagopus.log  result.txt

  out/20150331163451/test_async:
  lagopus.log  result.txt
```
