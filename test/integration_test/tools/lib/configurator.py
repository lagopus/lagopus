import os
import sys
import yaml

# const.
# config items.
# conf names.
CONF_SEC_SYS = "system"

CONF_SEC_TARGET = "target"
CONF_SEC_TESTER = "tester"
CONF_DPID = "dpid"
CONF_HOST = "host"
CONF_IS_REMOTE = "is_remote"
CONF_USER = "user"
CONF_LAGOPUS_PATH = "lagopus_path"
CONF_LAGOPUS_DSL = "lagopus_dsl"
CONF_LAGOPUS_OPTS = "lagopus_opts"

CONF_SEC_OFP = "ofp"
CONF_SEC_DS = "datastore"
CONF_PORT = "port"
CONF_VERSION = "version"


class ConfigParser:
    def __init__(self):
        self.confs = {}

    def load(self, file):
        with open(file) as f:
            self.confs = yaml.load(f)

    def get_confs(self):
        return self.confs

# config parser obj.
config_parser = ConfigParser()
