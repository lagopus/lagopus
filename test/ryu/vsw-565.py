""" write_actions test. """

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

    @set_ev_cls(EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        """Handle switch features reply to install flow entries."""
        datapath = ev.msg.datapath
        print 'installing sample flow'
#        [self.install_sample2(datapath, n) for n in [0]]
        [self.install_sample(datapath, n) for n in [0]]

    def create_match(self, parser, fields):
        """Create OFP match struct from the list of fields."""
        match = parser.OFPMatch()
        for (field, value) in fields.iteritems():
            match.append_field(field, value)
        return match

    def create_flow_mod(self, datapath, priority,
                        table_id, match, instructions):
        """Create OFP flow mod message."""
        ofproto = datapath.ofproto
        flow_mod = datapath.ofproto_parser.OFPFlowMod(datapath, 0, 0, table_id,
                                                      ofproto.OFPFC_ADD, 0, 0,
                                                      priority,
                                                      ofproto.OFPCML_NO_BUFFER,
                                                      ofproto.OFPP_ANY,
                                                      OFPG_ANY, 0,
                                                      match, instructions)
        return flow_mod

    def install_sample(self, datapath, table_id):
        """Create and install sample flow entries."""
        parser = datapath.ofproto_parser
        ofproto = datapath.ofproto

        # table 2: output to controller.
        match = self.create_match(parser, {})
        output = parser.OFPActionOutput(ofproto.OFPP_CONTROLLER, 0)
        write = parser.OFPInstructionActions(ofproto.OFPIT_WRITE_ACTIONS,
                                             [output])
        instructions = [write]
        flow_mod = self.create_flow_mod(datapath, 0, 2,
                                        match, instructions)
        datapath.send_msg(flow_mod)

        # table 1: push vlan and goto table 2
        match = self.create_match(parser, {})
        gototable = parser.OFPInstructionGotoTable(2)
        instructions = [gototable]
        flow_mod = self.create_flow_mod(datapath, 0, 1,
                                        match, instructions)
        datapath.send_msg(flow_mod)

        # table 0: push vlan and goto table 1
        match = self.create_match(parser, {})
        gototable = parser.OFPInstructionGotoTable(1)
        instructions = [gototable]
        flow_mod = self.create_flow_mod(datapath, 0, 0,
                                        match, instructions)
        datapath.send_msg(flow_mod)

        # packet-out message
        print "Send Packet-Out"
        in_port = 1
        actions = [parser.OFPActionOutput(ofproto.OFPP_TABLE, 0)]
        data = '\x00\x00\x00\x00\x00\x00x00\x00\x00\x00\x00\x00\x08\x00'
        req = parser.OFPPacketOut(datapath, ofproto.OFP_NO_BUFFER,
                                  in_port, actions, data)
        datapath.send_msg(req)
