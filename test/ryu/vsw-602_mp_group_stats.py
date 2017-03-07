from ryu.base.app_manager import RyuApp
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.ofp_event import EventOFPGroupStatsReply
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

        self.logger.info("=== start ===")
        for i in range(10000):
            port = 1
            max_len = 2000
            actions = [parser.OFPActionOutput(port, max_len)]
            weight = 100
            watch_port = 0
            watch_group = 0
            buckets = [parser.OFPBucket(weight, watch_port, watch_group,
                                        actions)]
            group_id = i
            req = parser.OFPGroupMod(datapath, ofproto.OFPGC_ADD,
                                     ofproto.OFPGT_SELECT, group_id, buckets)
            datapath.send_msg(req)

        self.logger.info("=== end ===")

        req = parser.OFPGroupStatsRequest(datapath, 0, ofproto.OFPG_ALL)
        datapath.send_msg(req)


    @set_ev_cls(EventOFPGroupStatsReply, MAIN_DISPATCHER)
    def group_stats_reply_handler(self, ev):
        global c
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
