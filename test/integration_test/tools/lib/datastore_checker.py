#!/usr/bin/env python

import sys
import os
import logging
import time
import copy
import json
from contextlib import contextmanager
from nose.tools import eq_

# lib tool
from const import *
from datastore_cmd import DataStoreCmd

_datastore_checker = None


class DataStoreChecker(object):

    def __init__(self, connections=None, is_tls=False,
                 certfile=None, keyfile=None, ca_certs=None):
        self.connections = connections if connections else {0x01:
                                                            {"host": "127.0.0.1",
                                                             "port": 12345}}
        self.dscs = {}
        self.is_tls = is_tls
        self.certfile = certfile
        self.keyfile = keyfile
        self.ca_certs = ca_certs

    def start(self):
        for dpid, c in self.connections.items():
            self.dscs[dpid] = DataStoreCmd(host=c["host"], port=c["port"],
                                           is_tls=self.is_tls,
                                           certfile=self.certfile,
                                           keyfile=self.keyfile,
                                           ca_certs=self.ca_certs)
            self.dscs[dpid].create_sock()
            self.dscs[dpid].connect()

    def close(self):
        for dsc in self.dscs.values():
            dsc.close()

    def cmp(self, cmd, expected, actual):
        logging.debug("cmd: %s" % cmd)
        logging.debug("expected: %s" % expected)
        logging.debug("actual: %s" % actual)

        eq_(expected, actual)

    def cmp_json(self, cmd, expected, actual):
        for i in [expected, actual]:
            if isinstance(i, list):
                i.sort()

        self.cmp(cmd, expected, actual)

    def exec_cmd(self, dpid, cmd, expected_res, timeout=DEFAULT_TIMEOUT):
        res = self.dscs[dpid].send_cmd(cmd, timeout)

        expected_res_json = json.loads(expected_res)
        if expected_res:
            self.cmp_json(cmd, expected_res_json, res)

        return expected_res_json, res


def datastore_checker_start(**kwds):
    global _datastore_checker
    if not _datastore_checker:
        _datastore_checker = DataStoreChecker(**kwds)
        _datastore_checker.start()
    return _datastore_checker


def datastore_checker_stop():
    global _datastore_checker
    if _datastore_checker:
        _datastore_checker.close()
        _datastore_checker = None


def datastore_checker_cmd(dpid, cmd, expected_res,
                          timeout=DEFAULT_TIMEOUT):
    return _datastore_checker.exec_cmd(dpid, cmd,
                                       expected_res, timeout)

if __name__ == "__main__":
    # tests
    # precondition: start lagopus (dpid:0x01).
    logging.basicConfig(level="DEBUG")

    dpid = 0x01
    datastore_checker_start(connections={dpid: {"host": "127.0.0.1",
                                                "port": 12345}})

    datastore_checker_cmd(0x01, "flow",
                          '{"ret":"OK",'
                          '"data":[{"name":":bridge01",'
                          '"tables":[{"table":0,'
                          '"flows":[]}]}]}')

    datastore_checker_stop()
