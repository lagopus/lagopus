import os
import sys
import unittest
import nose
import yaml
import copy
from jinja2 import Template

from checker import *
from ofp.ofp import ofp_creators

# test scenario items
SCE_TESTCASES = "testcases"
SCE_TESTCASE = "testcase"
SCE_LAGOPUS_DSL = "lagopus_dsl"
SCE_SETUP = "setup"
SCE_TEARDOWN = "teardown"
SCE_TEST = "test"
SCE_CMDS = "cmds"
SCE_CMD_TYPE = "cmd_type"
SCE_CMD = "cmd"
SCE_RESULT = "result"
SCE_TIMEOUT = "timeout"
SCE_SW = "sw"
SCE_REPETITION_COUNT = "repetition_count"

SCE_USE_LAGOPUS = "lagopus"
SCE_USE_OFP = "ofp"
SCE_USE_DS = "ds"

SCE_CMD_TYPE_DS = "ds"
SCE_CMD_TYPE_SH = "shell"
SCE_CMD_TYPE_OFP_SEND = "ofp_send"
SCE_CMD_TYPE_OFP_RECV = "ofp_recv"

SCE_SW_TARGET = "target"
SCE_SW_TESTER = "tester"

SCE_LAGOPUS_DSL_FILE = "file"

SCE_REPETITION_COUNT_DEFAULT = 1


class CmdExecuter:

    def replase_kws(self, string, replase_kws):
        return Template(string).render(replase_kws)

    def execute_ds(self, dpid, cmd, result, timeout):
        datastore_checker_cmd(dpid, cmd, result, timeout)

    def execute_shell(self, cmd, result, timeout):
        shell_checker_cmd(cmd, result, timeout)

    def execute_ofp_send(self, dpid, cmd, result, timeout):
        dp = ofp_checker_get_dp(dpid)
        ofp_type_str = cmd.keys()[0]
        # create ofp obj.
        msg = ofp_creators[ofp_type_str].create(self, dp, dp.ofproto,
                                                dp.ofproto_parser,
                                                cmd[ofp_type_str])
        # send.
        ofp_checker_send_msg(dpid, msg, timeout)

    def execute_ofp_recv(self, dpid, cmd, result, timeout):
        dp = ofp_checker_get_dp(dpid)
        ofp_type_str = result.keys()[0]
        # create ofp obj.
        msg = ofp_creators[ofp_type_str].create(self, dp, dp.ofproto,
                                                dp.ofproto_parser,
                                                result[ofp_type_str])

        # recv and reply check (xid is not checked).
        ofp_checker_recv_msg(dpid, msg, None, None, 0, timeout)

    def execute(self, dpid, type, cmd, result,
                timeout, replase_kws):
        # replase key words.
        c = cmd
        if cmd is not None:
            if isinstance(cmd, str):
                c = self.replase_kws(cmd, replase_kws)

        r = result
        if result is not None:
            if isinstance(result, int):
                result = str(result)
            if isinstance(result, str):
                r = self.replase_kws(result, replase_kws)

        if type == SCE_CMD_TYPE_DS:
            self.execute_ds(dpid, c, r, timeout)
        elif type == SCE_CMD_TYPE_SH:
            self.execute_shell(c, r, timeout)
        elif type == SCE_CMD_TYPE_OFP_SEND:
            self.execute_ofp_send(dpid, c, r, timeout)
        elif type == SCE_CMD_TYPE_OFP_RECV:
            self.execute_ofp_recv(dpid, c, r, timeout)

    def exec_cmd(self, test_case_obj, cmd):
        timout = DEFAULT_TIMEOUT
        dpid = test_case_obj.opts["switches"]["target_sw"]["dpid"]

        if SCE_SW in cmd:
            sw = "%s_sw" % cmd[SCE_SW]
            dpid = test_case_obj.opts["switches"][sw]["dpid"]

        if SCE_TIMEOUT in cmd:
            timout = cmd[SCE_TIMEOUT]

        c = None
        if SCE_CMD in cmd:
            c = cmd[SCE_CMD]

        r = None
        if SCE_RESULT in cmd:
            r = cmd[SCE_RESULT]

        self.execute(dpid, cmd[SCE_CMD_TYPE], c, r,
                     timout, test_case_obj.opts)

    def exec_cmds(self, test_case_obj, cmds):
        # get repetition count.
        repetition_count = SCE_REPETITION_COUNT_DEFAULT
        if SCE_REPETITION_COUNT in cmds:
            repetition_count = cmds[SCE_REPETITION_COUNT]

        for i in range(repetition_count):
            cs = copy.deepcopy(cmds)
            logging.debug("index: %d" % i)
            test_case_obj.opts["index"] = i
            for cmd in cs[SCE_CMDS]:
                self.exec_cmd(test_case_obj, cmd)


class YamlTestExecuter:

    def create_case(self, file, test_case):
        def do_case(self):
            for cmds in test_case[SCE_TEST]:
                self.cmd_executer.exec_cmds(self, cmds)

        return do_case

    def create_setup(self, file, scenario):
        def do_setUp(self):
            # add cleanup func.
            self.addCleanup(self.cleanups)

            checker_setup(self)
            self.opts = checker_get_opts(self)

            checker_start_lagopus(self)
            checker_start_datastore(self)

            # section: lagopus_dsl.
            if SCE_LAGOPUS_DSL in scenario:
                dpid = self.opts["switches"]["target_sw"]["dpid"]

                if SCE_SW in scenario[SCE_LAGOPUS_DSL]:
                    sw = "%s_sw" % scenario[SCE_LAGOPUS_DSL][SCE_SW]
                    dpid = self.opts["switches"][sw]["dpid"]

                # exec load command.
                self.cmd_executer.execute(
                    dpid, SCE_CMD_TYPE_DS,
                    "load %s" % scenario[SCE_LAGOPUS_DSL][
                        SCE_LAGOPUS_DSL_FILE],
                    '{"ret":"OK"}',
                    DEFAULT_TIMEOUT,
                    self.opts)
                checker_start_ofp(self)

            # section: setup
            if SCE_SETUP in scenario:
                for cmds in scenario[SCE_SETUP]:
                    self.cmd_executer.exec_cmds(self, cmds)

        return do_setUp

    def create_cleanups(self, file, scenario):
        def do_cleanups(self):
            # section: teardown
            if SCE_TEARDOWN in scenario:
                for cmds in scenario[SCE_TEARDOWN]:
                    self.cmd_executer.exec_cmds(self, cmds)

            # section: lagopus_dsl.
            if SCE_LAGOPUS_DSL in scenario:
                checker_stop_ofp(self)

            checker_stop_datastore(self)
            checker_stop_lagopus(self)
            checker_teardown(self)

        return do_cleanups

    def read_yaml(self, file):
        with open(file) as f:
            data = yaml.load(f)
        return data

    def create_suite(self, files=None):
        suite = unittest.TestSuite()

        for file in files:
            scenario = self.read_yaml(file)

            # create test case class.
            file_name = os.path.splitext(os.path.basename(file))[0]
            TestYaml = type('TestYaml%s' % file_name, (unittest.TestCase,), {})

            # setup method.
            ms = self.create_setup(file, scenario)
            setattr(TestYaml, "setUp",  ms)

            # cleanups method.
            mdc = self.create_cleanups(file, scenario)
            setattr(TestYaml, "cleanups",  mdc)

            # test case method.
            for test_case in scenario[SCE_TESTCASES]:
                name = test_case[SCE_TESTCASE].replace(" ", "_").lower()

                mc = self.create_case(file, test_case)
                mc.__name__ = 'test_yaml_%s_%s' % (file_name, name)
                mc.__doc__ = "%s (%s)" % (mc.__name__, file)
                setattr(TestYaml, mc.__name__, mc)
                mc = None

            # attr: yaml file name.
            setattr(TestYaml, "__lagopus_yaml__", file)
            setattr(TestYaml, "cmd_executer", CmdExecuter())

            suite.addTest(unittest.makeSuite(TestYaml))

        return suite

yaml_test_executer = YamlTestExecuter()
