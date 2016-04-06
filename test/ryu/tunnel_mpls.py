from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.handler import set_ev_cls
from ryu.controller.handler import CONFIG_DISPATCHER
from ryu.ofproto.ofproto_v1_3 import OFP_VERSION

class TunnelMpls(RyuApp):
    OFP_VERSIONS = [OFP_VERSION]

    def __init__(self, *args, **kwargs):
        super(TunnelMpls, self).__init__(*args, **kwargs)

    @set_ev_cls(EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath
        self.logger.info('installing flow')
        self.install_flow(datapath)

    def install_flow(self, datapath):
        ofp = datapath.ofproto
        ofp_parser = datapath.ofproto_parser
        self.install_flow_encap(datapath, ofp, ofp_parser)
        self.install_flow_decap(datapath, ofp, ofp_parser)

    def install_flow_encap(self, datapath, ofp, ofp_parser):
        # encap mpls
        self.logger.info('installing encap')
        cookie = cookie_mask = 0
        table_id = 0
        idle_timeout = hard_timeout = 0
        buffer_id = ofp.OFP_NO_BUFFER
        priority = 313
        match = ofp_parser.OFPMatch(eth_type=2048)
        actions = [ofp_parser.OFPActionEncap(100423),
                   ofp_parser.OFPActionSetField(mpls_label=1),
                   ofp_parser.OFPActionSetField(mpls_tc=0),
                   ofp_parser.OFPActionSetField(mpls_bos=1),
                   ofp_parser.OFPActionSetNwTtl(64),
                   ofp_parser.OFPActionEncap(0),
                   ofp_parser.OFPActionSetField(eth_dst="22:33:33:33:33:33"),
                   ofp_parser.OFPActionSetField(eth_src="12:22:22:22:22:22"),
                   ofp_parser.OFPActionOutput(2)]
        inst = [ofp_parser.OFPInstructionActions(ofp.OFPIT_APPLY_ACTIONS,
                                                 actions)]
        req = ofp_parser.OFPFlowMod(datapath, cookie, cookie_mask,
                                    table_id, ofp.OFPFC_ADD,
                                    idle_timeout, hard_timeout,
                                    priority, buffer_id,
                                    ofp.OFPP_ANY, ofp.OFPG_ANY,
                                    ofp.OFPFF_SEND_FLOW_REM,
                                    match, inst)
        datapath.send_msg(req)

    def install_flow_decap(self, datapath, ofp, ofp_parser):
        # decap mpls
        self.logger.info('installing decap')
        cookie = cookie_mask = 0
        table_id = 0
        idle_timeout = hard_timeout = 0
        buffer_id = ofp.OFP_NO_BUFFER
        priority = 314
        match = ofp_parser.OFPMatch(eth_type=34887)
        actions = [ofp_parser.OFPActionDecap(cur_pkt_type=0, new_pkt_type=100423),
                   ofp_parser.OFPActionDecap(cur_pkt_type=100423, new_pkt_type=0),
                   ofp_parser.OFPActionOutput(2)]
        inst = [ofp_parser.OFPInstructionActions(ofp.OFPIT_APPLY_ACTIONS,
                                                 actions)]
        req = ofp_parser.OFPFlowMod(datapath, cookie, cookie_mask,
                                    table_id, ofp.OFPFC_ADD,
                                    idle_timeout, hard_timeout,
                                    priority, buffer_id,
                                    ofp.OFPP_ANY, ofp.OFPG_ANY,
                                    ofp.OFPFF_SEND_FLOW_REM,
                                    match, inst)
        datapath.send_msg(req)
