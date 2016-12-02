"""VXLAN VNI match test."""

import time
from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.ofp_event import EventOFPPacketIn
from ryu.controller.handler import set_ev_cls
from ryu.controller.handler import CONFIG_DISPATCHER
from ryu.controller.handler import MAIN_DISPATCHER
from ryu.ofproto.ofproto_v1_2 import OFPG_ANY
from ryu.ofproto.ofproto_v1_3 import OFP_VERSION
from ryu.lib.mac import haddr_to_bin


class L2Switch(RyuApp):
    OFP_VERSIONS = [OFP_VERSION]

    def __init__(self, *args, **kwargs):
        super(L2Switch, self).__init__(*args, **kwargs)
        self.msgs = {}

    @set_ev_cls(EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        """Handle switch features reply to install flow entries."""
        datapath = ev.msg.datapath
        print 'installing sample flow'
#        [self.install_sample2(datapath, n) for n in [0]]
        [self.install_sample(datapath, n) for n in [0]]
#        [self.flow_stats(datapath, n) for n in [0]]

    def create_match(self, parser, fields):
        """Create OFP match struct from the list of fields."""
        match = parser.OFPMatch()
        for (field, value) in fields.iteritems():
            match.append_field(field, value)
        return match

    def install_sample(self, datapath, table_id):
        """Create and install sample flow entries."""
        ofp_parser = datapath.ofproto_parser
        ofp = datapath.ofproto

        print "Register FlowMod"
        match = self.create_match(ofp_parser,
                                  {ofp.OXM_OF_ETH_TYPE: 0x0800,
                                   ofp.OXM_OF_IP_PROTO: 17,
                                   ofp.OXM_OF_UDP_DST: 4789,
                                   ofp.OXM_OF_VXLAN_VNI: 10})
        actions = [ofp_parser.OFPActionOutput(ofp.OFPP_CONTROLLER, 0)]
        inst = [ofp_parser.OFPInstructionActions(ofp.OFPIT_APPLY_ACTIONS,
                                                 actions)]
        req = ofp_parser.OFPFlowMod(datapath, 0, 0, 0, ofp.OFPFC_ADD, 0, 0,
                                    100, ofp.OFP_NO_BUFFER,
                                    ofp.OFPP_ANY, ofp.OFPG_ANY, 0,
                                    match, inst)
        datapath.send_msg(req)

        # packet-out message
        print "Send Packet-Out"
        in_port = 1
        actions = [ofp_parser.OFPActionOutput(ofp.OFPP_TABLE, 0)]
        data = '\x00\x00\x00\x00\x00\x00' + '\x00\x00\x00\x00\x00\x00' + \
            '\x08\x00' + \
            '\x45\x00\x00\x00\x00\x00\x00\x00\xff' + \
            '\x11\x00\x00\xc0\x00\x00\x01\xc0\x00\x00\x02' + \
            '\x00\x00\x12\xb5\x00\x00\x00\x00' + \
            '\x00\x00\x00\x00' + '\x00\x00\x0a'
        req = ofp_parser.OFPPacketOut(datapath, ofp.OFP_NO_BUFFER,
                                      in_port, actions, data)
        datapath.send_msg(req)

    @set_ev_cls(EventOFPPacketIn, MAIN_DISPATCHER)
    def packet_in_handler(self, ev):
        """Handle packet_in events."""
        msg = ev.msg
        datapath = msg.datapath
        ofp = datapath.ofproto
        if msg.reason == ofp.OFPR_NO_MATCH:
            reason = 'NO MATCH'
        elif msg.reason == ofp.OFPR_ACTION:
            reason = 'ACTION'
        elif msg.reason == ofp.OFPR_INVALID_TTL:
            reason = 'INVALID TTL'
        else:
            reason = 'unknown'

        print 'OFPPacketIn received: '
        print ' buffer_id=%x' % msg.buffer_id
        print ' total_len=%d' % msg.total_len
        print ' reason=%s' % reason
        print ' table_id=%d' % msg.table_id
        print ' cookie=%d' % msg.cookie
        print ' match=%s' % msg.match
        print ' data=%s' % repr(msg.data)
