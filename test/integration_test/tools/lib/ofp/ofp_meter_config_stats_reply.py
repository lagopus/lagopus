import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_meter_config_stats import SCE_METER_CONFIG_STATS
from .ofp_meter_config_stats import OfpMeterConfigStatsCreator

# YAML:
# meter_config_stats_reply:
#   flags: 0
#   body:
#     - meter_config_stats:
#        flags: 0
#        meter_id: 0
#        bands:
#          - drop
#            rate: 0
#            burst_size: 0

SCE_METER_CONFIG_STATS_REPLY = "meter_config_stats_reply"
SCE_METER_CONFIG_STATS_BODY = "body"


@register_ofp_creators(SCE_METER_CONFIG_STATS_REPLY)
class OfpMeterConfigStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # MeterConfigStatsReply.
        kws = copy.deepcopy(params)

        body = []
        if SCE_METER_CONFIG_STATS_BODY in params:
            for config_stats in params[SCE_METER_CONFIG_STATS_BODY]:
                config_stats_obj = OfpMeterConfigStatsCreator.create(
                    test_case_obj, dp,
                    ofproto, ofp_parser,
                    config_stats[SCE_METER_CONFIG_STATS])
                body.append(config_stats_obj)
        kws[SCE_METER_CONFIG_STATS_BODY] = body

        # create MeterConfigStatsReply.
        msg = ofp_parser.OFPMeterConfigStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_METER_CONFIG

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
