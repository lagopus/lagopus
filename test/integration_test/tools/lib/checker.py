import os
import sys
import logging

from ofp_checker import ofp_checker_start, ofp_checker_stop
from ofp_checker import ofp_checker_get_dp, ofp_checker_get_dps
from ofp_checker import ofp_checker_send_msg, ofp_checker_recv_msg
from ofp_checker import OFPChecker
from datastore_checker import datastore_checker_start, datastore_checker_stop
from datastore_checker import datastore_checker_cmd
from datastore_checker import DataStoreChecker
from shell_checker import lagopus_checker_start, lagopus_checker_stop
from shell_checker import shell_checker_cmd
from shell_checker import ShellChecker
from const import *
from opt_parser import *
from configurator import *


def mk_out_dir(test_case_obj, opts):
    # out dir path.
    module_path = ""
    if hasattr(test_case_obj, "__lagopus_yaml__"):
        # yaml test case.
        module_path = test_case_obj.__lagopus_yaml__
    else:
        # nose test case.
        module_path = os.path.abspath(sys.modules.get(
            test_case_obj.__module__).__file__)
    target_path = opts.target[0]
    test_method = test_case_obj._testMethodName

    m = ""
    if os.path.isfile(target_path):
        m = os.path.splitext(os.path.basename(module_path))[0]
    elif os.path.isdir(target_path):
        m = os.path.splitext(
            module_path.replace(target_path + os.sep, "", 1))[0]
    else:
        opt_parser.usage()

    out_dir = os.path.join(opts.out_dir, m, test_method)
    os.makedirs(out_dir)

    return out_dir


def checker_get_opts(test_case_obj):
    return test_case_obj.checker_opts


def checker_setup(test_case_obj, is_use_tester=False):
    logging.info("checker setup.")

    ops = opt_parser.get_opts()
    confs = config_parser.get_confs()
    out_dir = mk_out_dir(test_case_obj, ops)

    # set opts.
    opts = {}
    opts["test_home"] = ops.test_home
    opts["out_dir"] = out_dir
    opts["is_use_tester"] = is_use_tester
    opts["ofp"] = {"port": confs[CONF_SEC_OFP][CONF_PORT],
                   "version": confs[CONF_SEC_OFP][CONF_VERSION]}
    opts["datastore"] = {"port": confs[CONF_SEC_DS][CONF_PORT]}

    secs = [CONF_SEC_TARGET]
    if is_use_tester:
        secs += [CONF_SEC_TESTER]

    opts["switches"] = {}
    for sec in secs:
        opts["switches"][sec] = {
            "dpid": confs[sec][CONF_DPID],
            "host": confs[sec][CONF_HOST],
            "user": confs[sec][CONF_USER],
            "lagopus": {"is_remote": confs[sec][CONF_IS_REMOTE],
                        "path": confs[sec][CONF_LAGOPUS_PATH],
                        "dsl": confs[sec][CONF_LAGOPUS_DSL],
                        "opts": confs[sec][CONF_LAGOPUS_OPTS],
                        "logdir": out_dir,
                        "logname": "lagopus.log"}}

    test_case_obj.checker_opts = opts


def checker_start_lagopus(test_case_obj, timeout=DEFAULT_TIMEOUT):
    logging.info("start lagopus.")

    opts = test_case_obj.checker_opts
    confs = {}
    for opt in opts["switches"].values():
        confs[opt["dpid"]] = opt["lagopus"]
        confs[opt["dpid"]].update({"host": opt["host"],
                                   "user": opt["user"]})

    lagopus_checker_start(confs, timeout)


def checker_start_datastore(test_case_obj):
    logging.info("start datastore.")

    opts = test_case_obj.checker_opts
    connections = {}
    for opt in opts["switches"].values():
        connections[opt["dpid"]] = {"host": opt["host"],
                                    "port": opts["datastore"]["port"]}

    datastore_checker_start(connections=connections)


def checker_start_ofp(test_case_obj, timeout=DEFAULT_TIMEOUT):
    logging.info("start ofp.")

    opts = test_case_obj.checker_opts
    dpids = []
    for opt in opts["switches"].values():
        dpids += [opt["dpid"]]

    ofp_checker_start(dpids=dpids,
                      port=opts["ofp"]["port"],
                      version=opts["ofp"]["version"],
                      timeout=timeout)


def checker_teardown(test_case_obj):
    logging.info("checker teardown.")


def checker_stop_lagopus(test_case_obj, timeout=DEFAULT_TIMEOUT):
    logging.info("stop lagopus.")
    lagopus_checker_stop(timeout)


def checker_stop_datastore(test_case_obj):
    logging.info("stop datastore.")
    datastore_checker_stop()


def checker_stop_ofp(test_case_obj):
    logging.info("stop ofp.")
    ofp_checker_stop()
