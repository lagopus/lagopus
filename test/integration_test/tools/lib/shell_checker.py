#!/usr/bin/env python

import os
import sys
import logging
from nose.tools import eq_

from shell_cmd import ShellCmd, ShellCmdTimeOut, exec_sh_cmd
from const import *
from opt_parser import *
from configurator import *

_lagopus_checker = None


class ShellChecker(object):

    def __init__(self):
        pass

    def cmp(self, cmd, expected, actual):
        if isinstance(expected, int):
            expected = str(expected)
        if isinstance(actual, int):
            actual = str(actual)

        logging.debug("cmd: %s" % cmd)
        logging.debug("expected: %s" % expected)
        logging.debug("actual: %s" % actual)

        eq_(expected, actual)

    def test_cmd(self, cmd, expected_res, timeout=30):
        logging.debug("exec sh: %s" % cmd)
        actual_res = exec_sh_cmd(cmd, timeout)

        # check result code.
        if expected_res is not None:
            self.cmp(cmd, expected_res, actual_res)


def shell_checker_cmd(cmd, expected_res, timeout=30):
    sc = ShellChecker()
    sc.test_cmd(cmd, expected_res, timeout)


class LagopusChecker(ShellChecker):

    def __init__(self, confs):
        super(LagopusChecker, self).__init__()
        self.confs = confs

    def exec_lagopus(self, action, timeout=DEFAULT_TIMEOUT):
        for conf in self.confs.values():
            if conf["is_remote"]:
                host = conf["host"]
                logdir = "/tmp"
                local_logdir = conf["logdir"]
                opts = ""
            else:
                host = "127.0.0.1"
                logdir = conf["logdir"]
                local_logdir = ""
                opts = "-C %s" % os.path.abspath(conf["dsl"])

            opts += " %s" % conf["opts"]
            cmd = "%s %s %s %s %d %s '%s' %s %s" % (
                os.path.join(SHELL_HOME,
                             "%s_lagopus_ansible.sh" % action),
                os.path.abspath(conf["path"]),
                conf["user"],
                host,
                timeout,
                logdir,
                local_logdir,
                conf["logname"],
                opts)

            self.test_cmd(cmd, 0, timeout)

    def start(self, timeout=DEFAULT_TIMEOUT):
        self.exec_lagopus("start", timeout)

    def stop(self, timeout=DEFAULT_TIMEOUT):
        self.exec_lagopus("stop", timeout)

def lagopus_checker_start(confs, timeout=DEFAULT_TIMEOUT):
    global _lagopus_checker
    if not _lagopus_checker:
        _lagopus_checker = LagopusChecker(confs)
    _lagopus_checker.start(timeout)
    return _lagopus_checker


def lagopus_checker_stop(timeout=DEFAULT_TIMEOUT):
    global _lagopus_checker
    if _lagopus_checker:
        _lagopus_checker.stop(timeout)
        _lagopus_checker = None
