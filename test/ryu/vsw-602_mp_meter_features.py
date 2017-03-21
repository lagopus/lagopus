from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.ofp_event import EventOFPMeterFeaturesStatsReply
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

        req = parser.OFPMeterFeaturesStatsRequest(datapath, 0)
        datapath.send_msg(req)

    @set_ev_cls(EventOFPMeterFeaturesStatsReply, MAIN_DISPATCHER)
    def meter_features_stats_reply_handler(self, ev):
        features = []
        for stat in ev.msg.body:
            features.append('max_meter=%d band_types=0x%08x '
                            'capabilities=0x%08x max_bands=%d '
                            'max_color=%d' %
                            (stat.max_meter, stat.band_types,
                             stat.capabilities, stat.max_bands,
                             stat.max_color))
        self.logger.info('MeterFeaturesStats: %s', features)
