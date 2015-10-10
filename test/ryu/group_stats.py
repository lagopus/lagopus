from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.ofp_event import EventOFPGroupStatsReply
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

    def group_mod(self, datapath):
        ofp = datapath.ofproto
        ofp_parser = datapath.ofproto_parser

        port = 1
        max_len = 2000
        actions = [ofp_parser.OFPActionOutput(port, max_len)]
        weight = 100
        watch_port = 0
        watch_group = 0
        buckets = [ofp_parser.OFPBucket(weight, watch_port, watch_group,
                                        actions)]
        group_id = 1

        req = ofp_parser.OFPGroupMod(datapath, ofp.OFPGC_ADD,
                                     ofp.OFPGT_SELECT, group_id, buckets)

        datapath.send_msg(req)

    def group_stats_request(self, datapath):
        ofp = datapath.ofproto
        ofp_parser = datapath.ofproto_parser

        req = ofp_parser.OFPGroupStatsRequest(datapath, 0, ofp.OFPG_ALL)

        datapath.send_msg(req)

    def install(self, datapath):
        self.group_mod(datapath)
        self.group_stats_request(datapath)

    @set_ev_cls(EventOFPGroupStatsReply, MAIN_DISPATCHER)
    def group_stats_reply_handler(self, ev):
        groups = []
        for stat in ev.msg.body:
            groups.append('length=%d group_id=%d '
                          'ref_count=%d packet_count=%d byte_count=%d '
                          'duration_sec=%d duration_nsec=%d' %
                          (stat.length, stat.group_id,
                           stat.ref_count, stat.packet_count,
                           stat.byte_count, stat.duration_sec,
                           stat.duration_nsec))
        self.logger.info('GroupStats: %s', groups)

    @set_ev_cls(EventOFPErrorMsg,
                [HANDSHAKE_DISPATCHER, CONFIG_DISPATCHER, MAIN_DISPATCHER])
    def error_msg_handler(self, ev):
        msg = ev.msg
        self.logger.info('OFPErrorMsg received: type=0x%02x code=0x%02x '
                         'message=%s',
                         msg.type, msg.code, hex_array(msg.data))
