from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.ofp_event import EventOFPGroupFeaturesStatsReply
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
        self.logger.info('installing sample flow')
        [self.install_sample(datapath, n) for n in [0]]

    def install_sample(self, datapath, table_id):
        parser = datapath.ofproto_parser
        ofproto = datapath.ofproto
        req = parser.OFPGroupFeaturesStatsRequest(datapath, 0)
        datapath.send_msg(req)

    @set_ev_cls(EventOFPGroupFeaturesStatsReply, MAIN_DISPATCHER)
    def group_features_stats_reply_handler(self, ev):
        body = ev.msg.body
        self.logger.info('GroupFeaturesStats: types=%d '
                         'capabilities=0x%08x max_groups=%s '
                          'actions=%s',
                         body.types, body.capabilities,
                         body.max_groups, body.actions)
