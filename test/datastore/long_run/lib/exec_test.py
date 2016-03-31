#!/usr/bin/env python

import os
import sys
import six
import json
import logging
import socket
import traceback
import copy
import time
from contextlib import contextmanager
from jinja2 import Template

from datastore_cmd import DataStoreCmd, open_ds_cmd
from async_datastore_cmd import AsyncDataStoreCmd, open_async_ds_cmd
from shell_cmd import ShellCmd, ShellCmdTimeOut, exec_sh_cmd
from const import *


class DataStoreTestError(Exception):
    pass


class ExecCmd(object):

    def replase_kws(self, string, replase_kws):
        return Template(string).render(replase_kws)

    def cmp(self, cmd, expected, actual):
        logging.debug("expected: %s" % expected)
        logging.debug("actual: %s" % actual)

        if expected != actual:
            raise DataStoreTestError(
                "\t\tcmd: %s,\n\t\texpected: %s,\n\t\tactual: %s" %
                (cmd, expected, actual))


class ExecShellCmd(ExecCmd):

    def __init__(self):
        ExecCmd.__init__(self)

        run_func_targets = ["lagopus", "ryu"]
        for target in run_func_targets:
            func_str = "run_%s" % target.lower()
            self.__dict__[func_str] = self.create_run_func(target)

    def cmp_sh_res(self, cmd, expected, actual):
        if isinstance(expected, int):
            expected = str(expected)
        if isinstance(actual, int):
            actual = str(actual)

        self.cmp(cmd, expected, actual)

    def exec_cmd(self, cmd, result, timeout, replase_kws):
        # replase key words.
        c = self.replase_kws(cmd, replase_kws)
        r = None
        if result is not None:
            r = self.replase_kws(result, replase_kws)

        logging.debug("exec sh: %s" % c)
        # exec shell cmd.
        res = exec_sh_cmd(c, timeout)

        # check result code.
        if result is not None:
            self.cmp_sh_res(c, r, res)

    def start_cmd(self, target, replase_kws):
        cmd = "%s/start_%s.sh %s %s %s %s" % (replase_kws["shell_dir"],
                                              target,
                                              replase_kws["%s_path" % target],
                                              replase_kws["log_dir"],
                                              replase_kws["%s_log" % target],
                                              replase_kws["%s_opts" % target])
        result = "0"
        self.exec_cmd(cmd, result, SCE_TIMEOUT_DEFAULT, replase_kws)

    def stop_cmd(self, target, replase_kws):
        cmd = "%s/stop_%s.sh %s %d" % (replase_kws["shell_dir"],
                                       target,
                                       replase_kws["log_dir"],
                                       SCE_TIMEOUT_DEFAULT)
        result = "0"
        self.exec_cmd(cmd, result, SCE_TIMEOUT_DEFAULT, replase_kws)

    def create_run_func(self, target):
        # create run_lagopus(), run_ryu()
        @contextmanager
        def run_func(scenario, replase_kws):
            is_used = False
            if SCE_USE in scenario:
                # check is_use (lagopus, ryu)
                sce_use_str = "SCE_USE_%s" % target.upper()
                if globals()[sce_use_str] in scenario[SCE_USE]:
                    is_used = True
            try:
                if is_used:
                    self.start_cmd(target, replase_kws)
                yield
            finally:
                if is_used:
                    self.stop_cmd(target, replase_kws)
        return run_func


class ExecDsCmd(ExecCmd):

    def __init__(self):
        ExecCmd.__init__(self)

    def cmp_json(self, cmd, expected, actual):
        for i in [expected, actual]:
            if isinstance(i, list):
                i.sort()

        self.cmp(cmd, expected, actual)

    def exec_cmd(self, dsc, cmd, result, timeout, replase_kws):
        # replase key words.
        c = self.replase_kws(cmd, replase_kws)
        r = None
        if result is not None:
            r = self.replase_kws(result, replase_kws)

        logging.debug("send cmd: %s" % c)

        if isinstance(dsc, DataStoreCmd):
            # sync mode.
            # send datastore cmd.
            res = dsc.send_cmd(c, timeout)

            # check response.
            if result is not None:
                self.cmp_json(c, json.loads(r), res)
        else:
            # async mode.
            # send datastore cmd.
            dsc.send_cmd(c)


class ExecTest(object):

    def __init__(self, opts, confs, test_home, file, out_dir):
        self.exec_ds_cmd = ExecDsCmd()
        self.exec_shell_cmd = ExecShellCmd()
        self.opts = opts
        self.confs = confs
        self.name = os.path.splitext(os.path.basename(file))[0]
        self.out_dir = os.path.join(
            out_dir, self.name)
        self.result_file = os.path.join(self.out_dir, RESULT_FILE)
        self.mode_funcs = {SCE_MODE_SYNC: open_ds_cmd,
                           SCE_MODE_ASYNC: open_async_ds_cmd}
        self.result = []
        self.dsc_kws = {
            "is_tls": self.confs.getboolean(CONF_SEC_TLS, CONF_SEC_TLS_IS_ENABLE),
            "certfile":  self.confs.get(CONF_SEC_TLS, CONF_SEC_TLS_CERTFILE),
            "keyfile": self.confs.get(CONF_SEC_TLS, CONF_SEC_TLS_KEYFILE),
            "ca_certs": self.confs.get(CONF_SEC_TLS, CONF_SEC_TLS_CA_CERTS),
            "host": HOST,  # Constant value
            "port": self.confs.getint(CONF_SEC_SYS, CONF_SEC_SYS_DS_PORT),
        }
        self.replase_kws = {
            "scenario": self.name,  # scenario name
            "testcase": "",  # testcase name
            "test_home": test_home,  # this test home path.
            "shell_dir": os.path.join(test_home, SH_DIR),
            "lagopus_path": self.confs.get(CONF_SEC_LAGOPUS, CONF_SEC_LAGOPUS_PATH),
            "lagopus_opts": "\"-C %s %s\"" % (
                os.path.join(test_home,
                             self.confs.get(CONF_SEC_LAGOPUS, CONF_SEC_LAGOPUS_CONF)),
                self.confs.get(CONF_SEC_LAGOPUS, CONF_SEC_LAGOPUS_OPTS)),
            "lagopus_log": LAGOPUS_LOG,
            "ryu_path": self.confs.get(CONF_SEC_RYU, CONF_SEC_RYU_PATH),
            "ryu_opts": "\"%s %s\"" % (
                self.confs.get(CONF_SEC_RYU, CONF_SEC_RYU_OPTS),
                os.path.join(test_home, self.confs.get(CONF_SEC_RYU,
                                                       CONF_SEC_RYU_APP))),
            "ryu_log": RYU_LOG,
            "log_dir": self.out_dir,
            "index": 0,  # repetition count index
        }

    def write(self, f, string):
        f.write(string + "\n")
        six.print_(string)

    def exec_cmd(self, dsc, cmd):
        # get timeout.
        timeout = SCE_TIMEOUT_DEFAULT
        if SCE_TIMEOUT in cmd:
            timeout = cmd[SCE_TIMEOUT]

        result = cmd[SCE_RESULT] if SCE_RESULT in cmd else None
        if cmd[SCE_CMD_TYPE] == SCE_CMD_TYPE_DS:
            # exec datastore cmd.
            self.exec_ds_cmd.exec_cmd(dsc, cmd[SCE_CMD],
                                      result, timeout,
                                      self.replase_kws)
        else:
            # exec shell cmd.
            self.exec_shell_cmd.exec_cmd(cmd[SCE_CMD],
                                         result, timeout,
                                         self.replase_kws)
            pass

    def exec_cmds(self, dsc, cmds):
        # get repetition count.
        repetition_count = SCE_REPETITION_COUNT_DEFAULT
        if SCE_REPETITION_COUNT in cmds:
            repetition_count = cmds[SCE_REPETITION_COUNT]

        # get repetition time.
        repetition_time = None
        if SCE_REPETITION_TIME in cmds:
            # minutes
            repetition_time = cmds[SCE_REPETITION_TIME] * 60

        i = 0
        start_time = time.time()
        while True:
            if repetition_time is None:
                if i >= repetition_count:
                    break
            else:
                remaining_time = time.time() - start_time
                if remaining_time > repetition_time:
                    break

            logging.debug("index: %d" % i)
            self.replase_kws["index"] = i
            for cmd in cmds[SCE_CMDS]:
                self.exec_cmd(dsc, cmd)
            i += 1

    def exec_tests(self, dsc, testcase):
        for cmds in testcase:
            self.exec_cmds(dsc, cmds)

    def exec_scenario_tests(self, scenario, testcase, cmd_func):
        err_str = ""
        with cmd_func(**self.dsc_kws) as dsc:
            try:
                # setup.
                if SCE_SETUP in scenario:
                    self.exec_tests(dsc, scenario[SCE_SETUP])

                # tests.
                self.exec_tests(dsc, testcase[SCE_TEST])
            except DataStoreTestError as e:
                # ERROR
                err_str = "%s" % (e)
            except (socket.timeout, ShellCmdTimeOut):
                # sock or shell timeout.
                err_str = "%s" % (traceback.format_exc())
            finally:
                try:
                    # teardown.
                    if SCE_TEARDOWN in scenario:
                        self.exec_tests(dsc, scenario[SCE_TEARDOWN])
                except (DataStoreTestError, socket.timeout, ShellCmdTimeOut):
                    # ERROR
                    if err_str == "":
                        err_str = "%s" % (traceback.format_exc())
        return err_str

    def exec_scenario(self, scenario, f):
        for testcase in scenario[SCE_TESTCASES]:
            self.replase_kws["testcase"] = testcase[SCE_TESTCASE]

            mode = SCE_MODE_SYNC
            if SCE_MODE in scenario:
                mode = scenario[SCE_MODE]
            self.write(f, "mode: %s" % mode)

            cmd_func = self.mode_funcs[mode]
            # run lagopus/ryu.
            with self.exec_shell_cmd.run_ryu(scenario, self.replase_kws):
                with self.exec_shell_cmd.run_lagopus(scenario, self.replase_kws):
                    # exec tests.
                    err_str = self.exec_scenario_tests(
                        scenario, testcase, cmd_func)

                # check error.
                if err_str == "":
                    self.result.append(RET_OK)
                    result = "  %-90s  %s" % (self.replase_kws["testcase"],
                                              RET_OK)
                else:
                    self.result.append(RET_ERROR)
                    result = "  %-90s  %s\n%s" % (self.replase_kws["testcase"],
                                                  RET_ERROR, err_str)

                self.write(f, result)

    def run(self, scenario):
        os.makedirs(self.out_dir)
        six.print_("out dir: %s" % self.out_dir)

        with open(self.result_file, "w") as f:
            # run test.
            self.write(f, "scenario: %s" % self.name)

            self.exec_scenario(scenario, f)
