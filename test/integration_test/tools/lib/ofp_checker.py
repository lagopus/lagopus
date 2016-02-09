#!/usr/bin/env python

import sys
import os
import logging
import time
import copy
from contextlib import contextmanager
from types import *
from nose.tools import eq_

# lib ryu
from ryu.ofproto import ofproto_parser
from ryu.ofproto import ofproto_v1_3
from ryu.ofproto import ofproto_protocol

# lib tool
from const import *
from ofp_datapath import OFPDatapath, OFPServer

# OFPMatch hook.
def _match_eq_hook(self, obj):
    if not obj.fields:
        buf = bytearray()
        obj.serialize(buf, 0)
        obj = obj.parser(buf, 0)
    return obj

# ofp Comparison func.
def _ofp_eq_func(self, other):
    targets_tmp = []
    objs = {"self": self, "other": other}

    for k, v in objs.items():
        if hasattr(v, "_ofp_eq_hook"):
            objs[k] = self._ofp_eq_hook(v)

    # target name.
    for obj in objs.values():
        if hasattr(obj, "_targets"):
            targets_tmp += obj._targets
    targets = list(set(targets_tmp))

    for obj in objs.values():
        obj._set_targets(targets)

    # get atters.
    self_atters = {k: v for k, v in self.stringify_attrs()}
    other_atters = {k: v for k, v in other.stringify_attrs()}

    logging.debug("expected : %s : %s" % (objs["self"].__class__.__name__,
                                          self_atters))
    logging.debug("actual   : %s : %s" % (objs["other"].__class__.__name__,
                                          other_atters))

    ret = self_atters == other_atters
    if ret == False:
        logging.error("expected : %s : %s" % (objs["self"].__class__.__name__,
                                              self_atters))
        logging.error("actual   : %s : %s" % (objs["other"].__class__.__name__,
                                              other_atters))

    return ret


def _ofp_stringify_attrs(self):
    for k, v in self.stringify_attrs_ori():
        if hasattr(self, "_targets") and self._targets:
            if k not in self._targets:
                continue
        yield (k, v)


def _ofp_ne_func(self, other):
    return not self.__eq__(other)


def _set_targets_func(self, targets=None):
    self._targets = targets if targets else []


def set_hooks():
    ofproto_parser.StringifyMixin.__eq__ = _ofp_eq_func
    ofproto_parser.StringifyMixin.__ne__ = _ofp_ne_func
    ofproto_parser.StringifyMixin._set_targets = _set_targets_func
    ofproto_parser.StringifyMixin._targets = []
    ofproto_parser.StringifyMixin.__repr__ = ofproto_parser.StringifyMixin.__str__
    ofproto_parser.StringifyMixin.stringify_attrs_ori = ofproto_parser.StringifyMixin.stringify_attrs
    ofproto_parser.StringifyMixin.stringify_attrs = _ofp_stringify_attrs

    for proto, parser in ofproto_protocol._versions.values():
        parser.OFPMatch._ofp_eq_hook = _match_eq_hook

set_hooks()

_ofp_checker = None


class OFPChecker(object):

    def __init__(self, version=ofproto_v1_3.OFP_VERSION,
                 dpids=None, host="0.0.0.0", port=6633, is_tls=False,
                 certfile=None, keyfile=None, ca_certs=None):

        self.server = OFPServer(host=host, port=port,
                                is_tls=is_tls, certfile=certfile,
                                keyfile=keyfile, ca_certs=ca_certs)

        dpids = dpids if dpids else [0x1]
        self.dps = {i: None for i in dpids}
        self.version = version

    def start(self, timeout=DEFAULT_TIMEOUT):
        self.server.create_sock()

        # undefined dpid
        dpid = 0x0
        while None in self.dps.values():
            csock = self.server.accept(timeout)

            self.dps[dpid] = OFPDatapath(csock, version=self.version)

            new_dpid = self.handshake(dpid)
            if new_dpid in self.dps.keys():
                self.dps[new_dpid] = self.dps[dpid]
                del self.dps[dpid]

    def close(self):
        for dp in self.dps.values():
            dp.close()
        self.server.close()

    def send(self, dpid, msg, timeout=DEFAULT_TIMEOUT):
        msg = self.dps[dpid].set_send_msg(msg)
        logging.debug("send msg: %s" % msg)
        self.dps[dpid].send_msg(msg, timeout)

    def recv(self, dpid, expected_msg=None, timeout=DEFAULT_TIMEOUT):
        start_time = time.time()
        while True:
            msg = self.dps[dpid].recvall(timeout)

            if msg:
                if expected_msg:
                    if type(expected_msg) == type(msg):
                        eq_(expected_msg, msg)
                        break
                else:
                    break

            remaining_time = time.time() - start_time
            if timeout and remaining_time > timeout:
                raise TimeOut("timed out")
            time.sleep(0.1)

        logging.debug("recv msg: %s" % msg)
        return msg

    def handshake(self, dpid):
        # send HELLO.
        sm = self.dps[dpid].ofproto_parser.OFPHello(self.dps[dpid])
        self.send_msg(dpid, sm)

        # recv HELLO.
        rm = self.dps[dpid].ofproto_parser.OFPHello(self.dps[dpid])
        self.recv_msg(dpid, rm,
                      ["version", "msg_type"],
                      None, None)

        # send FeaturesRequest.
        sm = self.dps[dpid].ofproto_parser.OFPFeaturesRequest(self.dps[dpid])
        self.send_msg(dpid, sm)

        # recv FeaturesReply.
        # create expected_msg.
        em = self.dps[dpid].ofproto_parser.OFPSwitchFeatures(self.dps[dpid],
                                                             auxiliary_id=0,
                                                             capabilities=0x4f,
                                                             datapath_id=None,
                                                             n_buffers=65535,
                                                             n_tables=255)
        em, rm = self.recv_msg(dpid, em,
                               ["version", "msg_type", "msg_len", "xid",
                                "auxiliary_id", "capabilities",
                                "n_buffers", "n_tables"],
                               sm.xid, 32)
        return rm.datapath_id

    def create_expected_msg(self, dpid, ofp_msg,
                            ofp_func_targets=None,
                            xid=0, msg_len=0):
        ofp_msg.set_headers(ofp_msg.datapath.ofproto.OFP_VERSION,
                            ofp_msg.cls_msg_type,
                            msg_len,
                            xid)
        if ofp_func_targets:
            ofp_msg._set_targets(ofp_func_targets)

        return ofp_msg

    def send_msg(self, dpid, ofp_msg, timeout=DEFAULT_TIMEOUT):
        self.send(dpid, ofp_msg)
        return ofp_msg

    def recv_msg(self, dpid, ofp_msg,
                 ofp_func_targets=None,
                 xid=0, msg_len=0,
                 timeout=DEFAULT_TIMEOUT):
        e_ofp_msg = self.create_expected_msg(dpid, ofp_msg,
                                             ofp_func_targets,
                                             xid, msg_len)
        r_ofp_msg = self.recv(dpid, e_ofp_msg)
        return e_ofp_msg, r_ofp_msg

    def get_dp(self, dpid):
        return self.dps[dpid]

    def get_dps(self):
        return self.dps


def ofp_checker_start(timeout=DEFAULT_TIMEOUT, **kwds):
    global _ofp_checker
    if not _ofp_checker:
        _ofp_checker = OFPChecker(**kwds)
        _ofp_checker.start(timeout)
    return _ofp_checker


def ofp_checker_stop():
    global _ofp_checker
    if _ofp_checker:
        _ofp_checker.close()
        _ofp_checker = None


def ofp_checker_get_dp(dpid):
    return _ofp_checker.get_dp(dpid)


def ofp_checker_get_dps():
    return _ofp_checker.get_dps()


def ofp_checker_send_msg(dpid, ofp_msg, timeout=DEFAULT_TIMEOUT):
    return _ofp_checker.send_msg(dpid, ofp_msg, timeout)


def ofp_checker_recv_msg(dpid, ofp_msg,
                         ofp_func_targets=None,
                         xid=0, msg_len=0,
                         timeout=DEFAULT_TIMEOUT):
    return _ofp_checker.recv_msg(dpid, ofp_msg,
                                 ofp_func_targets,
                                 xid, msg_len,
                                 timeout)

if __name__ == "__main__":
    # tests
    # precondition: start lagopus (dpid:0x01).
    logging.basicConfig(level="DEBUG")

    dpid = 0x01
    ofp_checker_start(dpids=[dpid])

    dp = ofp_checker_get_dp(dpid)
    ofp = dp.ofproto
    ofp_parser = dp.ofproto_parser

    # send FlowMod.
    match = ofp_parser.OFPMatch(in_port=1, eth_dst='ff:ff:ff:ff:ff:ff')
    actions = [ofp_parser.OFPActionOutput(ofp.OFPP_NORMAL, 0)]
    inst = [ofp_parser.OFPInstructionActions(ofp.OFPIT_APPLY_ACTIONS,
                                             actions)]
    sm = ofp_parser.OFPFlowMod(dp, cookie=0,
                               cookie_mask=0,
                               table_id=0,
                               command=ofp.OFPFC_ADD,
                               idle_timeout=0,
                               hard_timeout=0,
                               priority=10,
                               buffer_id=ofp.OFP_NO_BUFFER,
                               out_port=0,
                               out_group=0,
                               flags=0,
                               match=match,
                               instructions=inst)
    sm = ofp_checker_send_msg(dpid, sm)

    # send FlowStatsRequest.
    sm = ofp_parser.OFPFlowStatsRequest(dp, flags=0,
                                        table_id=ofp.OFPTT_ALL,
                                        out_port=ofp.OFPP_ANY,
                                        out_group=ofp.OFPG_ANY,
                                        cookie=0,
                                        cookie_mask=0,
                                        match=match)
    sm = ofp_checker_send_msg(dpid, sm)

    # recv FlowStatsReply.
    # create expected_msg.
    st = ofp_parser.OFPFlowStats(table_id=0, duration_sec=0,
                                 duration_nsec=0, priority=10,
                                 idle_timeout=0, hard_timeout=0, flags=0,
                                 cookie=0, packet_count=0, byte_count=0,
                                 match=match, instructions=inst)
    st.length = 96
    st._set_targets(["length", "table_id", "priority",
                     "idle_timeout", "hard_timeout", "flags",
                     "cookie", "packet_count", "byte_count",
                     "match", "instructions"])
    em = ofp_parser.OFPFlowStatsReply(dp, body=[st],
                                      flags=0)
    em.type = ofp.OFPMP_FLOW
    em, rm = ofp_checker_recv_msg(dpid, em, None,
                                  sm.xid, 112)

    ofp_checker_stop()
