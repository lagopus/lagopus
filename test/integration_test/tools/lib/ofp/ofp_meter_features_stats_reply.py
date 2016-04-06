import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_meter_features_stats import SCE_METER_FEATURES_STATS
from .ofp_meter_features_stats import OfpMeterFeaturesStatsCreator

# YAML:
# meter_features_stats_reply:
#   flags: 0
#   body:
#     - meter_features_stats:
#         max_meter: 0x0
#         band_types: 0x0
#         capabilities: 0x0
#         max_bands: 0x0
#         max_color: 0x0

SCE_METER_FEATURES_STATS_REPLY = "meter_features_stats_reply"
SCE_METER_FEATURES_STATS_BODY = "body"


@register_ofp_creators(SCE_METER_FEATURES_STATS_REPLY)
class OfpMeterFeaturesStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # MeterFeaturesStatsReply.
        kws = copy.deepcopy(params)

        body = []
        if SCE_METER_FEATURES_STATS_BODY in params:
            for stats in params[SCE_METER_FEATURES_STATS_BODY]:
                features_stats = OfpMeterFeaturesStatsCreator.create(
                    test_case_obj, dp,
                    ofproto, ofp_parser,
                    stats[SCE_METER_FEATURES_STATS])
                body.append(features_stats)
        kws[SCE_METER_FEATURES_STATS_BODY] = body

        # create MeterFeaturesStatsReply.
        msg = ofp_parser.OFPMeterFeaturesStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_METER_FEATURES

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
