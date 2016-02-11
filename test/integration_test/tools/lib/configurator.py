import os
import sys
import ConfigParser

# const.
# config items.
# conf names.
CONF_SEC_SYS = "system"

CONF_SEC_TARGET_SW = "target_sw"
CONF_SEC_TESTER_SW = "tester_sw"
CONF_DPID = "dpid"
CONF_HOST = "host"
CONF_LAGOPUS_PATH = "lagopus_path"
CONF_LAGOPUS_DSL = "lagopus_dsl"
CONF_LAGOPUS_OPTS = "lagopus_opts"

CONF_SEC_OFP = "ofp"
CONF_SEC_DS = "datastore"
CONF_PORT = "port"
CONF_VERSION = "version"

# config parser obj.
config_parser = ConfigParser.SafeConfigParser()
