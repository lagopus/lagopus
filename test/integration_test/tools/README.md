<!-- -*- mode: markdown -*- -->

How To Run "log run tests"
======================================
## Recommended
* Ubuntu 14.04

## Related packages
 * Python 2.7.6 or Python 3.4.3
 * pip
 * ansible 1.5.4
 * iproute
 * PyYAML 3.11
 * pykwalify 1.0.1
 * Jinja2 2.7.3
 * ryu 3.26
 * six 1.9.0
 * greenlet 0.4.5    (for ryu)
 * stevedore 1.3.0   (for ryu)

How to run test
---------------------------
## 1. Install packages.

```
 % sudo apt-get install python ansible python-dev python-pip iproute libxml2 libxslt-dev
 % sudo pip install --requirement ./requirements.pip
```

## 2. Install lagopus.
 cf.) {LAGOPUS\_DIR}/INSTALL.txt

## 3. Edit `lagopus_test.yml` file.

```
 % vi conf/lagopus_test.yml
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
 % ./lagopus_test.py -L debug sample_nose/test_ofp.py -- -v
   ----------------------------------------------------------------------
   Ran 1 test in 15.287s

   OK

```

  * YAML testcase.
```
 % ./lagopus_test.py -L debug -y sample_yaml/ofp/.yml -- -v
   ----------------------------------------------------------------------
   Ran 1 test in 15.287s

   OK

```

STATUS
---------------------------
* OFP
OFPT_HELLO
OFPT_ERROR
OFPT_ECHO_REQUEST ...  OK
OFPT_ECHO_REPLY ...  OK
OFPT_EXPERIMENTER
OFPT_FEATURES_REQUEST ...  OK
OFPT_FEATURES_REPLY ...  OK
OFPT_GET_CONFIG_REQUEST ...  OK
OFPT_GET_CONFIG_REPLY ...  OK
OFPT_SET_CONFIG ...  OK
OFPT_PACKET_IN ...  OK
OFPT_FLOW_REMOVED ...  OK
OFPT_PORT_STATUS ...  OK
OFPT_PACKET_OUT ...  OK
OFPT_FLOW_MOD ...  OK
OFPT_GROUP_MOD ...  OK
OFPT_PORT_MOD
OFPT_TABLE_MOD
OFPT_MULTIPART_REQUEST  ... -
OFPT_MULTIPART_REPLY ... -
OFPT_BARRIER_REQUEST ...  OK
OFPT_BARRIER_REPLY ...  OK
OFPT_QUEUE_GET_CONFIG_REQUEST ...  OK
OFPT_QUEUE_GET_CONFIG_REPLY ...  OK
OFPT_ROLE_REQUEST ...  OK
OFPT_ROLE_REPLY ...  OK
OFPT_GET_ASYNC_REQUEST ...  OK
OFPT_GET_ASYNC_REPLY ...  OK
OFPT_SET_ASYNC ...  OK
OFPT_METER_MOD ...  OK

* OFPT_MULTIPART
OFPMP_DESC ...  OK
OFPMP_FLOW ...  OK
OFPMP_AGGREGATE ...  OK
OFPMP_TABLE ...  OK
OFPMP_PORT_STATS ...  OK
OFPMP_QUEUE ...  OK
OFPMP_GROUP ...  OK
OFPMP_GROUP_DESC ...  OK
OFPMP_GROUP_FEATURES ...  OK
OFPMP_METER ...  OK
OFPMP_METER_CONFIG ...  OK
OFPMP_METER_FEATURES ...  OK
OFPMP_TABLE_FEATURES
OFPMP_PORT_DESC ...  OK
OFPMP_EXPERIMENTER
