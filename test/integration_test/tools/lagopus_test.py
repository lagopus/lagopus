#!/usr/bin/env python

import os
import os.path
import sys
import six
import nose
import logging
import datetime
from pykwalify.core import Core
from pykwalify.errors import SchemaError

# add load path.
for path in ["lib"]:
    sys.path.append(os.path.join(os.path.abspath(
        os.path.dirname(__file__)), path))

from opt_parser import *
from const import *
from configurator import *
from yaml_test_executer import *


def check_schema_test(opts, file):
    logging.info("check schema...: %s" % file)
    try:
        c = Core(source_file=file, schema_files=[opts.yaml_schema])
        c.validate(raise_exception=True)
    except SchemaError as e:
        six.print_("check schema: %-80s  ERROR" % file)
        raise
    else:
        six.print_("check schema: %-80s  OK" % file)


def check_schema_tests(opts, dir):
    for root, dirs, files in os.walk(dir):
        for file in files:
            if os.path.splitext(file)[1] not in YAML_SUFFIXES:
                continue
            file = os.path.join(root, file)
            check_schema_test(opts, file)


def parse_confs(opts):
    # configs
    config_parser.load(opts.config)


def get_yaml_files(path):
    fs = []
    if os.path.isfile(path):
        fs.append(os.path.abspath(path))
    elif os.path.isdir(path):
        for root, dirs, files in os.walk(path):
            for file in files:
                if os.path.splitext(file)[1] not in YAML_SUFFIXES:
                    continue
                fs.append(os.path.join(root, file))
    return fs

if __name__ == "__main__":
    date = datetime.datetime.today().strftime("%Y%m%d%H%M%S")
    test_home = os.path.abspath(os.path.dirname(__file__))

    # opts
    opts = opt_parser.parse_opts(test_home, date)

    debug_level_str = opts.log_level.upper()
    logging.basicConfig(
        filename=opts.log,
        format="[%(asctime)s][%(levelname)s]:%(filename)s:"
        "%(funcName)s:%(lineno)d:%(message)s",
        level=getattr(logging, debug_level_str))

    # configs
    parse_confs(opts)

    # first element is empty in argv.
    argv = ["", "--nocapture", "--logging-level", debug_level_str]
    argv += opts.nose_opts
    nose_run_args = {}
    if opts.yaml_schema is None:
        if opts.use_yaml:
            # format of YAML.
            files = get_yaml_files(opts.target[0])
            suite = yaml_test_executer.create_suite(files)
            nose_run_args["suite"] = suite
            nose_run_args["argv"] = argv
        else:
            # format of nose.
            nose_run_args["defaultTest"] = opts.target
            nose_run_args["argv"] = argv

        # run nose
        nose.run(**nose_run_args)
    else:
        # check YAML schema.
        if os.path.isfile(opts.target[0]):
            check_schema_test(opts, opts.target[0])
        elif os.path.isdir(opts.target[0]):
            check_schema_tests(opts, opts.target[0])
        else:
            opt_parser.usage()
