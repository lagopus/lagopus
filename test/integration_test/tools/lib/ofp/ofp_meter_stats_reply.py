import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_meter_stats import SCE_METER_STATS
from .ofp_meter_stats import OfpMeterStatsCreator

# YAML:
# meter_stats_reply:
#   flags: 0
#   body:
#     - meter_stats:
#        meter_id: 0
#        flow_count: 0
#        packet_in_count: 0
#        byte_in_count: 0
#        duration_sec: 0
#        duration_nsec: 0
#        band_stats:
#          - band_stats:
#              packet_band_count: 0
#              byte_band_count: 0

SCE_METER_STATS_REPLY = "meter_stats_reply"
SCE_METER_STATS_BODY = "body"


@register_ofp_creators(SCE_METER_STATS_REPLY)
class OfpMeterStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # MeterStatsReply.
        kws = copy.deepcopy(params)

        body = []
        if SCE_METER_STATS_BODY in params:
            for stats in params[SCE_METER_STATS_BODY]:
                stats_obj = OfpMeterStatsCreator.create(test_case_obj, dp,
                                                        ofproto, ofp_parser,
                                                        stats[SCE_METER_STATS])
                body.append(stats_obj)
        kws[SCE_METER_STATS_BODY] = body

        # create MeterStatsReply.
        msg = ofp_parser.OFPMeterStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_METER

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
