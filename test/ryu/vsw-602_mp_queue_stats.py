from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.ofp_event import EventOFPQueueStatsReply
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

        req = parser.OFPQueueStatsRequest(datapath, 0, ofproto.OFPP_ANY,
                                          ofproto.OFPQ_ALL)
        datapath.send_msg(req)

    @set_ev_cls(EventOFPQueueStatsReply, MAIN_DISPATCHER)
    def queue_stats_reply_handler(self, ev):
        queues = []
        for stat in ev.msg.body:
            queues.append('port_no=%d queue_id=%d '
                          'tx_bytes=%d tx_packets=%d tx_errors=%d '
                          'duration_sec=%d duration_nsec=%d' %
                          (stat.port_no, stat.queue_id,
                           stat.tx_bytes, stat.tx_packets, stat.tx_errors,
                           stat.duration_sec, stat.duration_nsec))
        self.logger.info('QueueStats: %s', queues)
