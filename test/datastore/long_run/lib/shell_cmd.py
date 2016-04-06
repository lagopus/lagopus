#!/usr/bin/env python

import os
import sys
import six
import json
import logging
import time
import subprocess
import threading
from const import *


class ShellCmdTimeOut(Exception):
    pass


class ShellCmd(object):

    def __init__(self, cmd):
        self.cmd = cmd
        self.process = None

    def exec_command(self, timeout):
        self.process = subprocess.Popen(self.cmd,
                                        shell=True,
                                        stderr=subprocess.STDOUT,
                                        stdout=subprocess.PIPE)

        # check timeout.
        start_time = time.time()
        while True:
            if self.process.poll() is not None:
                break
            remaining_time = time.time() - start_time
            if timeout and remaining_time > timeout:
                self.process.kill()
                # get stdout.
                stdout = self.process.communicate()[0]
                if stdout:
                    logging.debug("sh output: %s" % stdout)
                raise ShellCmdTimeOut("timed out, cmd: %s" % self.cmd)
            time.sleep(0.1)

        # get stdout.
        stdout = self.process.communicate()[0]
        if stdout:
            logging.debug("sh output: %s" % stdout)

        return self.process.returncode


def exec_sh_cmd(cmd, timeout):
    sc = ShellCmd(cmd)
    return sc.exec_command(timeout)

if __name__ == "__main__":
    # tests
    six.print_(exec_sh_cmd("ls -l", None))

    try:
        six.print_(exec_sh_cmd("sleep 10", 1))
    except ShellCmdTimeOut as e:
        six.print_(e)
