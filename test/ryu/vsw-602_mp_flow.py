from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.ofp_event import EventOFPFlowStatsReply
from ryu.controller.handler import set_ev_cls
from ryu.controller.handler import CONFIG_DISPATCHER
from ryu.controller.handler import MAIN_DISPATCHER
from ryu.ofproto.ofproto_v1_2 import OFPG_ANY
from ryu.ofproto.ofproto_v1_3 import OFP_VERSION
from ryu.lib.mac import haddr_to_bin

class App(RyuApp):
    OFP_VERSIONS = [OFP_VERSION]

    def __init__(self, *args, **kwargs):
        super(App, self).__init__(*args, **kwargs)

    @set_ev_cls(EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath
        [self.install_sample(datapath, n) for n in [0]]

    def create_match(self, parser, fields):
        match = parser.OFPMatch()
        for (field, value) in fields.iteritems():
            match.append_field(field, value)
        return match

    def create_flow_mod(self, datapath, priority,
                        table_id, match, instructions):
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
        in_port = 1
        eth_dst = '\x00' + '\x00' + '\x00' + '\x00' + '\x00' + '\x00'

        self.logger.info("=== start : flow mod ===")
        for i in range(1000):
            match = self.create_match(parser,
                                      {ofproto.OXM_OF_IN_PORT: in_port,
                                       ofproto.OXM_OF_ETH_DST: eth_dst,
                                       ofproto.OXM_OF_ETH_TYPE: 0x8847,
                                       ofproto.OXM_OF_MPLS_LABEL: i})
            popmpls = parser.OFPActionPopMpls(0x0800)
            decttl = parser.OFPActionDecNwTtl()
            pushvlan = parser.OFPActionPushVlan(0x8100)
            setvlanid = parser.OFPActionSetField(vlan_vid=1)
            setethsrc = parser.OFPActionSetField(eth_src='00:00:00:00:00:00')
            setethdst = parser.OFPActionSetField(eth_dst='00:00:00:00:00:00')
            output = parser.OFPActionOutput(2, 0)
            write = parser.OFPInstructionActions(ofproto.OFPIT_APPLY_ACTIONS,
                                                 [popmpls, decttl, pushvlan,
                                                  setvlanid, setethsrc,
                                                  output])

            instructions = [write]
            flow_mod = self.create_flow_mod(datapath, 100, table_id,
                                            match, instructions)
            datapath.send_msg(flow_mod)
        self.logger.info("=== end : flow mod ===")

        cookie = cookie_mask = 0
        match = parser.OFPMatch(in_port=1)
        req = parser.OFPFlowStatsRequest(datapath, 0,
                                         ofproto.OFPTT_ALL,
                                         ofproto.OFPP_ANY,
                                         ofproto.OFPG_ANY,
                                         cookie, cookie_mask,
                                         match)
        datapath.send_msg(req)

    @set_ev_cls(EventOFPFlowStatsReply, MAIN_DISPATCHER)
    def flow_stats_reply_handler(self, ev):
        flows = []
        for stat in ev.msg.body:
            flows.append('table_id=%s '
                         'duration_sec=%d duration_nsec=%d '
                         'priority=%d '
                         'idle_timeout=%d hard_timeout=%d flags=0x%04x '
                         'cookie=%d packet_count=%d byte_count=%d '
                         'match=%s instructions=%s' %
                         (stat.table_id,
                          stat.duration_sec, stat.duration_nsec,
                          stat.priority,
                          stat.idle_timeout, stat.hard_timeout, stat.flags,
                          stat.cookie, stat.packet_count, stat.byte_count,
                          stat.match, stat.instructions))
        self.logger.info('FlowStats: %s', flows)
