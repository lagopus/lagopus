from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.ofp_event import EventOFPQueueStatsReply
from ryu.controller.ofp_event import EventOFPErrorMsg
from ryu.controller.handler import set_ev_cls
from ryu.controller.handler import CONFIG_DISPATCHER
from ryu.controller.handler import MAIN_DISPATCHER
from ryu.controller.handler import HANDSHAKE_DISPATCHER
from ryu.ofproto.ofproto_v1_3 import OFP_VERSION
from ryu.utils import hex_array


class Stats(RyuApp):
    OFP_VERSIONS = [OFP_VERSION]

    def __init__(self, *args, **kwargs):
        super(Stats, self).__init__(*args, **kwargs)

    @set_ev_cls(EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        datapath = ev.msg.datapath
        self.logger.info('sending ...')
        self.install(datapath)

    def queue_stats_request(self, datapath):
        ofp = datapath.ofproto
        ofp_parser = datapath.ofproto_parser

        req = ofp_parser.OFPQueueStatsRequest(datapath, 0, ofp.OFPP_ANY,
                                              ofp.OFPQ_ALL)

        datapath.send_msg(req)

    def install(self, datapath):
        self.queue_stats_request(datapath)

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

    @set_ev_cls(EventOFPErrorMsg,
                [HANDSHAKE_DISPATCHER, CONFIG_DISPATCHER, MAIN_DISPATCHER])
    def error_msg_handler(self, ev):
        msg = ev.msg
        self.logger.info('OFPErrorMsg received: type=0x%02x code=0x%02x '
                         'message=%s',
                         msg.type, msg.code, hex_array(msg.data))
