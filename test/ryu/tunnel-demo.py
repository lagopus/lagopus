from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.handler import set_ev_cls
from ryu.controller.handler import CONFIG_DISPATCHER
from ryu.controller.handler import MAIN_DISPATCHER
from ryu.ofproto.ofproto_v1_2 import OFPG_ANY
from ryu.ofproto.ofproto_v1_3 import OFP_VERSION

class L2Switch(RyuApp):
    OFP_VERSIONS = [OFP_VERSION]

    def __init__(self, *args, **kwargs):
        super(L2Switch, self).__init__(*args, **kwargs)

    @set_ev_cls(EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        """Handle switch features reply to install table miss flow entries."""
        datapath = ev.msg.datapath
        print 'installing sample flow'
        self.install_sample(datapath)

    def create_flow_mod(self, datapath, match, instructions):
        """Create OFP flow mod message."""
        ofp = datapath.ofproto
        flow_mod = datapath.ofproto_parser.OFPFlowMod(datapath, 0, 0, 0,
                                                      ofp.OFPFC_ADD, 0, 0,
                                                      100,
                                                      ofp.OFPCML_NO_BUFFER,
                                                      ofp.OFPP_ANY,
                                                      OFPG_ANY, 0,
                                                      match, instructions)
        return flow_mod

    def install_sample(self, datapath):
        """Create and install sample flow entries."""
        ofp = datapath.ofproto
        parser = datapath.ofproto_parser
        peer_eth = '32:29:1d:5b:33:c1'
        my_eth = '0a:b3:c7:05:b3:19'
        peer_ip = '10.1.0.1'
        my_ip = '10.1.0.2'
        type_eth = (ofp.OFPHTN_ONF << 16) | ofp.OFPHTO_ETHERNET
        type_ip = (ofp.OFPHTN_ETHERTYPE << 16) | 0x0800
        type_gre = (ofp.OFPHTN_IP_PROTO << 16) | 47
        type_next = (ofp.OFPHTN_ONF << 16) | ofp.OFPHTO_USE_NEXT_PROTO
        call_id = 1
        # Incoming port 1.
        match = parser.OFPMatch(in_port=1,
                                eth_type=0x0800,
                                ipv4_src=peer_ip,
                                ipv4_dst=my_ip,
                                ip_proto=47,
                                gre_key=call_id)
        decap_eth = parser.OFPActionDecap(type_eth, type_ip)
        decap_ip = parser.OFPActionDecap(type_ip, type_gre)
        decap_gre = parser.OFPActionDecap(type_gre, type_next)
        output = parser.OFPActionOutput(2, 0)
        write = parser.OFPInstructionActions(ofp.OFPIT_APPLY_ACTIONS,
                                             [decap_eth,
                                              decap_ip,
                                              decap_gre,
                                              output])
        instructions = [write]
        flow_mod = self.create_flow_mod(datapath, match, instructions)
        datapath.send_msg(flow_mod)

        # Incoming port 2.
        in_port = 2
        match = parser.OFPMatch(in_port=2)
        encap_gre = parser.OFPActionEncap(type_gre)
        set_grekey = parser.OFPActionSetField(gre_key=call_id)
        encap_ip = parser.OFPActionEncap(type_ip)
        set_ipsrc = parser.OFPActionSetField(ipv4_src=my_ip)
        set_ipdst = parser.OFPActionSetField(ipv4_dst=peer_ip)
        encap_eth = parser.OFPActionEncap(type_eth)
        set_ethsrc = parser.OFPActionSetField(eth_src=my_eth)
        set_ethdst = parser.OFPActionSetField(eth_dst=peer_eth)
        output = parser.OFPActionOutput(1, 0)
        write = parser.OFPInstructionActions(ofp.OFPIT_APPLY_ACTIONS,
                                             [encap_gre,
                                              set_grekey,
                                              encap_ip,
                                              set_ipsrc,
                                              set_ipdst,
                                              encap_eth,
                                              set_ethsrc,
                                              set_ethdst,
                                              output])
        instructions = [write]
        flow_mod = self.create_flow_mod(datapath, match, instructions)
        datapath.send_msg(flow_mod)

        print "Done flow_mod."

