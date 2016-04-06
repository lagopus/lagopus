import os
import sys
import logging
from unittest import TestCase
from nose.tools import ok_, eq_

from checker import *


class OFPTest(TestCase):

    def setUp(self):
        # add cleanup func.
        self.addCleanup(self.cleanups)

        checker_setup(self)
        self.opts = checker_get_opts(self)

        # start
        checker_start_lagopus(self)
        checker_start_datastore(self)
        self.setup_ds()
        checker_start_ofp(self)

    def cleanups(self):
        # stop
        checker_stop_ofp(self)
        checker_stop_datastore(self)
        checker_stop_lagopus(self)

        checker_teardown(self)

    def setup_ds(self):
        e_res = '{"ret":"OK"}'
        dpid = self.opts["switches"]["target"]["dpid"]

        # TODO: type = ethernet-dpdk-phy
        cmds = ["channel channel01 create -dst-addr 127.0.0.1 -protocol tcp",
                "controller controller01 create -channel channel01",
                "interface interface01 create -type ethernet-dpdk-phy "
                "-port-number 0",
                "interface interface02 create -type ethernet-dpdk-phy "
                "-port-number 1",
                "interface interface03 create -type ethernet-dpdk-phy "
                "-port-number 2",
                "port port01 create -interface interface01",
                "port port02 create -interface interface02",
                "port port03 create -interface interface03",
                "bridge bridge01 create -controller controller01 "
                "-port port01 1 -port port02 2 -port port03 3 -dpid 0x1",
                "bridge bridge01 enable"]
        for cmd in cmds:
            datastore_checker_cmd(dpid, cmd, e_res)

    def test_flow(self):
        dpid = self.opts["switches"]["target"]["dpid"]
        dp = ofp_checker_get_dp(dpid)
        ofp = dp.ofproto
        ofp_parser = dp.ofproto_parser

        # send FlowMod.
        match = ofp_parser.OFPMatch(in_port=1, eth_dst="ff:ff:ff:ff:ff:ff")
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

        # send/rcv flow datastore cmd.
        e_res = '''
        {"ret":"OK",
        "data":[{"name":":bridge01",
        "tables":[{
        "flows":[{
        "actions":[{
        "apply_actions":[{"output":"normal"}]}],
        "cookie":0,
        "dl_dst":"ff:ff:ff:ff:ff:ff",
        "hard_timeout":0,
        "idle_timeout":0,
        "in_port":1,
        "priority":10}],
        "table":0}]}]}
        '''
        datastore_checker_cmd(0x01, "flow", e_res)
