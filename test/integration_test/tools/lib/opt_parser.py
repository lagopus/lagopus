import os
import sys
from argparse import ArgumentParser

from const import *

# const.
# default option values.
OPT_CONFIG_FILE = "./conf/lagopus_test.yml"
OPT_OUT_DIR = "./out"
OPT_LOG_LEVEL = "info"


class OptParser:

    def __init__(self):
        self.parser = ArgumentParser(description="Lagopus integration tester.")
        self.opts = None

    def usage(self):
        self.parser.print_help()
        sys.exit(1)

    def parse_opts(self, test_home, date):
        # options
        self.parser.add_argument("target",
                                 nargs=1,
                                 metavar="TEST_SCENARIOS_FILE_OR_DIR",
                                 help="test scenario[s] (dir or file)")

        self.parser.add_argument("nose_opts",
                                 nargs="*",
                                 metavar="NOSE_OPTS",
                                 help="nosetests command line opts "
                                 "(see 'nosetests -h or -- -h'). "
                                 "e.g., '-- -v --nocapture'")

        self.parser.add_argument("-y",
                                 action="store_true",
                                 dest="use_yaml",
                                 default=False,
                                 help="use YAML test case")

        self.parser.add_argument("-c",
                                 action="store",
                                 type=str,
                                 dest="config",
                                 default=OPT_CONFIG_FILE,
                                 help="config file (default: %s)" % OPT_CONFIG_FILE)

        self.parser.add_argument("-d",
                                 action="store",
                                 type=str,
                                 dest="out_dir",
                                 default=OPT_OUT_DIR,
                                 help="out dir (default: %s)" % OPT_OUT_DIR)

        self.parser.add_argument("-l",
                                 action="store",
                                 type=str,
                                 dest="log",
                                 default="",
                                 help="log file (default: stdout)")

        self.parser.add_argument("-L",
                                 action="store",
                                 type=str,
                                 dest="log_level",
                                 default=OPT_LOG_LEVEL,
                                 choices=["debug", "info", "warning",
                                          "error", "critical"],
                                 help="log level (default: %s)" % OPT_LOG_LEVEL)

        self.parser.add_argument("-C",
                                 action="store",
                                 type=str,
                                 dest="yaml_schema",
                                 default=None,
                                 help="YAML schema check")

        self.opts = self.parser.parse_args()

        # set date for out dir.
        self.opts.out_dir = os.path.abspath(
            os.path.join(self.opts.out_dir, date))
        # set target abspath.
        self.opts.target = [os.path.abspath(p) for p in self.opts.target]
        # set test home
        setattr(self.opts, "test_home", test_home)

        return self.opts

    def get_opts(self):
        return self.opts

opt_parser = OptParser()
