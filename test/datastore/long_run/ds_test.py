#!/usr/bin/env python

import os
import os.path
import sys
import six
import yaml
import logging
import datetime
from six.moves import configparser
from argparse import ArgumentParser
from pykwalify.core import Core
from pykwalify.errors import SchemaError

# add load path.
sys.path.append(os.path.join(os.path.abspath(
    os.path.dirname(__file__)), "lib"))

from exec_test import ExecTest
from const import *


def usage(parser):
    parser.print_help()
    sys.exit(1)


def parce_confs(opts):
    # configs
    confs = configparser.SafeConfigParser()
    confs.read(opts.config)
    return confs


def parse_opts():
    # options
    parser = ArgumentParser(description="DataStore tester.")

    parser.add_argument("target",
                        nargs=1,
                        metavar="TEST_SCENARIOS_FILE_OR_DIR",
                        help="test scenario[s] (dir or file)")

    parser.add_argument("-c",
                        action="store",
                        type=str,
                        dest="config",
                        default=OPT_CONFIG_FILE,
                        help="config file (default: %s)" % OPT_CONFIG_FILE)

    parser.add_argument("-d",
                        action="store",
                        type=str,
                        dest="out_dir",
                        default=OPT_OUT_DIR,
                        help="out dir (default: %s)" % OPT_OUT_DIR)

    parser.add_argument("-l",
                        action="store",
                        type=str,
                        dest="log",
                        default="",
                        help="log file (default: stdout)")

    parser.add_argument("-L",
                        action="store",
                        type=str,
                        dest="log_level",
                        default=OPT_LOG_LEVEL,
                        choices=["debug", "info", "warning",
                                 "error", "critical"],
                        help="log level (default: %s)" % OPT_LOG_LEVEL)

    parser.add_argument("-C",
                        action="store",
                        type=str,
                        dest="yaml_schema",
                        default=None,
                        help="YAML schema check")

    opts = parser.parse_args()

    return parser, opts


def read_yaml(file):
    with open(file) as f:
        data = yaml.load(f)
        return data


def summary_tests(ets):
    ok = 0
    err = 0
    unknown = 0
    for et in ets:
        for res in et.result:
            if res == RET_OK:
                ok += 1
            elif res == RET_ERROR:
                err += 1
            else:
                unknown += 1

    all = ok + err + unknown
    s = "ALL(%d)/OK(%d)/ERROR(%d)/UNKNOWN(%d)" % (all, ok, err, unknown)
    six.print_("\n\n==============")
    six.print_(s)
    logging.info("summary: %s" % s)

    return err or unknown


def check_schema_test(opts, file):
    logging.info("check schema...: %s" % file)
    try:
        c = Core(source_file=file, schema_files=[opts.yaml_schema])
        c.validate(raise_exception=True)
    except SchemaError as e:
        six.print_("check schema: %-80s  %s" % (file, RET_ERROR))
        raise
    else:
        six.print_("check schema: %-80s  %s" % (file, RET_OK))


def check_schema_tests(opts, dir):
    for root, dirs, files in os.walk(dir):
        for file in files:
            if os.path.splitext(file)[1] not in SUFFIXES:
                continue
            file = os.path.join(root, file)
            check_schema_test(opts, file)


def run_test(opts, confs, test_home, file, out_dir):
    if out_dir is None:
        out_dir = os.path.join(
            opts.out_dir,
            datetime.datetime.today().strftime("%Y%m%d%H%M%S"))

    data = read_yaml(file)
    et = ExecTest(opts, confs, test_home, file, out_dir)
    et.run(data)
    return [et]


def run_tests(opts, confs, test_home, dir):
    date = datetime.datetime.today().strftime("%Y%m%d%H%M%S")
    ets = []
    for root, dirs, files in os.walk(dir):
        for file in files:
            if os.path.splitext(file)[1] not in SUFFIXES:
                continue
            out_dir = os.path.join(
                opts.out_dir, date,
                root.replace(dir + os.sep, "", 1))
            file = os.path.join(root, file)
            ets += run_test(opts, confs, test_home, file, out_dir)
    return ets

if __name__ == "__main__":
    parser, opts = parse_opts()

    logging.basicConfig(
        filename=opts.log,
        format="[%(asctime)s][%(levelname)s]:%(filename)s:"
        "%(funcName)s:%(lineno)d:%(message)s",
        level=getattr(logging, opts.log_level.upper()))

    confs = parce_confs(opts)
    test_home = os.path.abspath(os.path.dirname(__file__))

    if opts.yaml_schema is None:
        # run tests.
        if os.path.isfile(opts.target[0]):
            ets = run_test(opts, confs, test_home, opts.target[0], None)
        elif os.path.isdir(opts.target[0]):
            ets = run_tests(opts, confs, test_home, opts.target[0])
        else:
            usage(parser)

        # summary
        err = summary_tests(ets)
        if err:
            sys.exit(1)
    else:
        # check schema
        if os.path.isfile(opts.target[0]):
            check_schema_test(opts, opts.target[0])
        elif os.path.isdir(opts.target[0]):
            check_schema_tests(opts, opts.target[0])
        else:
            usage(parser)
