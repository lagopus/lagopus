import os
import sys
from argparse import ArgumentParser

from const import *

# const.
# default option values.
OPT_CONFIG_FILE = "./conf/lagopus_test.ini"
OPT_OUT_DIR = "./out"
OPT_LOG_LEVEL = "info"

# opt obj.
OPTS = None

_parser = ArgumentParser(description="Lagopus integration tester.")


def usage():
    _parser.print_help()
    sys.exit(1)


def parse_opts(date):
    # options

    _parser.add_argument("target",
                         nargs=1,
                         metavar="TEST_SCENARIOS_FILE_OR_DIR",
                         help="test scenario[s] (dir or file)")

    _parser.add_argument("nose_opts",
                         nargs="*",
                         metavar="NOSE_OPTS",
                         help="nosetests command line opts (see 'nosetests -h or -- -h'). "
                         "e.g., '-- -v --nocapture'")

    _parser.add_argument("-c",
                         action="store",
                         type=str,
                         dest="config",
                         default=OPT_CONFIG_FILE,
                         help="config file (default: %s)" % OPT_CONFIG_FILE)

    _parser.add_argument("-d",
                         action="store",
                         type=str,
                         dest="out_dir",
                         default=OPT_OUT_DIR,
                         help="out dir (default: %s)" % OPT_OUT_DIR)

    _parser.add_argument("-l",
                         action="store",
                         type=str,
                         dest="log",
                         default="",
                         help="log file (default: stdout)")

    _parser.add_argument("-L",
                         action="store",
                         type=str,
                         dest="log_level",
                         default=OPT_LOG_LEVEL,
                         choices=["debug", "info", "warning",
                                  "error", "critical"],
                         help="log level (default: %s)" % OPT_LOG_LEVEL)

    global OPTS
    OPTS = _parser.parse_args()

    # set date for out dir.
    OPTS.out_dir = os.path.abspath(os.path.join(OPTS.out_dir, date))
    # set target abspath.
    OPTS.target = [os.path.abspath(p) for p in OPTS.target]

    return _parser, OPTS
