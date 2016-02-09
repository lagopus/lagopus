<!-- -*- mode: markdown -*- -->

How To Run "log run tests"
======================================
## Recommended
* Ubuntu 14.04

## Related packages
 * Python 2.7.6
 * pip
 * PyYAML 3.11
 * pykwalify 1.0.1
 * Jinja2 2.7.3
 * ryu 3.26
 * six 1.9.0         (for ryu)
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

## 3. Edit `lagopus_test.ini` file.

```
 % vi conf/lagopus_test.ini
```

## 4. Run YAML schema check (for sample).

```
 % ./lagopus_test.py -C schema/schema.yml sample_yaml/ofp.yml
```

## 5. Run test (for sample).
* usage:
  * lagopus_test.py [-h] [-c CONFIG] [-d OUT_DIR] [-l LOG]  
                         [-L {debug,info,warning,error,critical}]  
                         TEST_SCENARIOS_FILE_OR_DIR [NOSE_OPTS [NOSE_OPTS ...]]
  * positional arguments:
    * TEST_SCENARIOS_FILE_OR_DIR    test scenario[s] (dir or file)
    * NOSE_OPTS                     nosetests command line opts  
                                    (see 'nosetests -h or -- -h'). e.g., '-- -v --nocapture'
  * optional arguments:
    * -h, --help            show this help message and exit
    * -y                    use YAML test case
    * -c CONFIG             config file (default: ./conf/lagopus_test.ini)
    * -d OUT_DIR            out dir (default: ./out)
    * -l LOG                log file (default: stdout)
    * -L {debug,info,warning,error,critical} log level (default: info)


* example:
  * nose testcase.
```
 % sudo ./lagopus_test.py -L debug sample_nose/test_ofp.py -- -v
   ----------------------------------------------------------------------
   Ran 1 test in 15.287s

   OK

```

  * YAML testcase.
```
 % sudo ./lagopus_test.py -L debug -y sample_yaml/ofp/.yml -- -v
   ----------------------------------------------------------------------
   Ran 1 test in 15.287s

   OK

```
