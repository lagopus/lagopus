"""Test flow_mod with hard timeout"""

from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
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

    def create_flow_add(self, datapath, priority, timeout,
                        table_id, match, instructions):
        """Create OFP flow mod message."""
        ofproto = datapath.ofproto
        flow_mod = datapath.ofproto_parser.OFPFlowMod(datapath, 0, 0, table_id,
                                                      ofproto.OFPFC_ADD,
                                                      timeout, timeout,
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
        # Incoming port 1.
        in_port = 1;
        for timeout in range(60, 1 ,-1):
            # Incoming Ethernet destination
            match = self.create_match(parser,
                                      {ofproto.OXM_OF_METADATA: timeout})
            # Output to port 2.
            output = parser.OFPActionOutput(2, 0)
            write = parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS,
                                                 [output])
            instructions = [write]
            flow_mod = self.create_flow_add(datapath, 100, timeout,
                                            table_id, match, instructions)
            datapath.send_msg(flow_mod)

        print "sent flow_mod"
