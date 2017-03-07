from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.ofp_event import EventOFPTableFeaturesStatsReply
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


    def install_sample(self, datapath, table_id):
        parser = datapath.ofproto_parser
        ofproto = datapath.ofproto

        for i in range(256):
            match = parser.OFPMatch()
            flow_mod = parser.OFPFlowMod(datapath, 0, 0, i,
                                         ofproto.OFPFC_ADD, 0, 0,
                                         i,
                                         ofproto.OFPCML_NO_BUFFER,
                                         ofproto.OFPP_ANY,
                                         ofproto.OFPG_ANY, 0,
                                         match, [])
            datapath.send_msg(flow_mod)

        req = parser.OFPTableFeaturesStatsRequest(datapath, 0, [])
        datapath.send_msg(req)

    @set_ev_cls(EventOFPTableFeaturesStatsReply, MAIN_DISPATCHER)
    def table_stats_reply_handler(self, ev):
        tables = []
        for stat in ev.msg.body:
            tables.append('table_id=%d' %
                          (stat.table_id))
        self.logger.info('TableFeaturesStats: %s', tables)
