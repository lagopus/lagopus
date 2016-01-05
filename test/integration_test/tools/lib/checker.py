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


def mk_out_dir(test_case_obj):
    # out dir path.
    module_path = os.path.abspath(sys.modules.get(
        test_case_obj.__module__).__file__)
    target_path = OPTS.target[0]
    test_method = test_case_obj._testMethodName

    m = ""
    if os.path.isfile(OPTS.target[0]):
        m = os.path.splitext(os.path.basename(module_path))[0]
    elif os.path.isdir(OPTS.target[0]):
        m = os.path.splitext(
            module_path.replace(target_path + os.sep, "", 1))[0]
    else:
        usage()

    out_dir = os.path.join(OPTS.out_dir, m, test_method)
    os.makedirs(out_dir)

    return out_dir


def checker_get_opts(test_case_obj):
    return test_case_obj.checker_opts


def checker_setup(test_case_obj, is_use_tester_sw=False):
    logging.info("checker setup.")

    out_dir = mk_out_dir(test_case_obj)

    # set opts.
    opts = {}
    opts["out_dir"] = out_dir
    opts["is_use_tester_sw"] = is_use_tester_sw
    opts["ofp"] = {"port": CONFS.getint(CONF_SEC_OFP, CONF_PORT),
                   "version": CONFS.getint(CONF_SEC_OFP, CONF_VERSION)}
    opts["datastore"] = {"port": CONFS.getint(CONF_SEC_DS, CONF_PORT)}

    secs = [CONF_SEC_TARGET_SW]
    if is_use_tester_sw:
        secs += [CONF_SEC_TESTER_SW]

    opts["switches"] = {}
    for sec in secs:
        opts["switches"][sec] = {
            "dpid": CONFS.getint(sec, CONF_DPID),
            "host": CONFS.get(sec, CONF_HOST),
            "lagopus": {"path": CONFS.get(sec, CONF_LAGOPUS_PATH),
                        "dsl": CONFS.get(sec, CONF_LAGOPUS_DSL),
                        "opts": CONFS.get(sec, CONF_LAGOPUS_OPTS),
                        "logdir": out_dir,
                        "logname": "lagopus.log"}}

    test_case_obj.checker_opts = opts


def checker_start_lagopus(test_case_obj, timeout=DEFAULT_TIMEOUT):
    logging.info("start lagopus.")

    opts = test_case_obj.checker_opts
    confs = {}
    for opt in opts["switches"].values():
        confs[opt["dpid"]] = opt["lagopus"]

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
