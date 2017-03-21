from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.ofp_event import EventOFPMeterConfigStatsReply
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

    def create_meter_mod(self, datapath, command, flags_, meter_id, bands):
        ofproto = datapath.ofproto
        ofp_parser = datapath.ofproto_parser
        meter_mod = ofp_parser.OFPMeterMod(datapath, command, flags_,
                                           meter_id, bands)
        return meter_mod

    def install_sample(self, datapath, table_id):
        parser = datapath.ofproto_parser
        ofproto = datapath.ofproto

        self.logger.info("=== start ===")
        for i in range(3000):
            band = parser.OFPMeterBandDrop(rate=10, burst_size=1)
            meter_mod = self.create_meter_mod(datapath,
                                              ofproto.OFPMC_ADD,
                                              ofproto.OFPMF_PKTPS,
                                              i,
                                              [band])
            datapath.send_msg(meter_mod)
        self.logger.info("=== end ===")

        req = parser.OFPMeterConfigStatsRequest(datapath, 0,
                                                ofproto.OFPM_ALL)
        datapath.send_msg(req)

    @set_ev_cls(EventOFPMeterConfigStatsReply, MAIN_DISPATCHER)
    def meter_config_stats_reply_handler(self, ev):
        configs = []
        for stat in ev.msg.body:
            configs.append('length=%d flags=0x%04x meter_id=0x%08x '
                           'bands=%s' %
                           (stat.length, stat.flags, stat.meter_id,
                            stat.bands))
        self.logger.info('MeterConfigStats: %s', configs)
