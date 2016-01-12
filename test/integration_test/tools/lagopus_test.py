#!/usr/bin/env python

import os
import os.path
import sys
import nose
import logging
import datetime

# add load path.
sys.path.append(os.path.join(os.path.abspath(
    os.path.dirname(__file__)), "lib"))

from opt_parser import *
from const import *
from configurator import *


def parce_confs(opts):
    # configs
    CONFS.read(opts.config)

if __name__ == "__main__":
    date = datetime.datetime.today().strftime("%Y%m%d%H%M%S")

    # opts
    parser, opts = parse_opts(date)

    debug_level_str = opts.log_level.upper()
    logging.basicConfig(
        filename=opts.log,
        format="[%(asctime)s][%(levelname)s]:%(filename)s:"
        "%(funcName)s:%(lineno)d:%(message)s",
        level=getattr(logging, debug_level_str))

    # configs
    parce_confs(opts)

    # run nose;
    # first element is empty in argv.
    argv = ["", "--nocapture", "--logging-level", debug_level_str]
    argv += opts.nose_opts
    nose.run(defaultTest=opts.target, argv=argv)
