import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_port_stats import SCE_PORT_STATS
from .ofp_port_stats import OfpPortStatsCreator

# YAML:
# port_stats_reply:
#   flags: 0
#   body:
#     - port_stats:
#         port_no: 0
#         rx_packets: 0
#         tx_packets: 0
#         rx_bytes: 0
#         tx_bytes: 0
#         rx_dropped: 0
#         tx_dropped: 0
#         rx_errors: 0
#         tx_errors: 0
#         rx_frame_err: 0
#         rx_over_err: 0
#         rx_crc_err: 0
#         collisions: 0
#         duration_sec: 0
#         duration_nsec: 0

SCE_PORT_STATS_REPLY = "port_stats_reply"
SCE_PORT_STATS_BODY = "body"


@register_ofp_creators(SCE_PORT_STATS_REPLY)
class OfpPortStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # PortStatsReply.
        kws = copy.deepcopy(params)

        body = []
        if SCE_PORT_STATS_BODY in params:
            for stats in params[SCE_PORT_STATS_BODY]:
                stats_obj = OfpPortStatsCreator.create(test_case_obj, dp,
                                                        ofproto, ofp_parser,
                                                        stats[SCE_PORT_STATS])
                body.append(stats_obj)
        kws[SCE_PORT_STATS_BODY] = body

        # create PortStatsReply.
        msg = ofp_parser.OFPPortStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_PORT_STATS

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
