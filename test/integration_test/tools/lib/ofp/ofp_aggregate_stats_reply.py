import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_aggregate_stats import SCE_AGGREGATE_STATS
from .ofp_aggregate_stats import OfpAggregateStatsCreator

# YAML:
# aggregate_stats_reply:
#   flags: 0
#   body:
#     aggregate_stats:
#       packet_count: 0
#       byte_count: 0
#       flow_count: 0

SCE_AGGREGATE_STATS_REPLY = "aggregate_stats_reply"
SCE_AGGREGATE_STATS_BODY = "body"


@register_ofp_creators(SCE_AGGREGATE_STATS_REPLY)
class OfpAggregateStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # AggregateStatsReply.
        kws = copy.deepcopy(params)

        body = None
        if SCE_AGGREGATE_STATS_BODY in params:
            if SCE_AGGREGATE_STATS in params[SCE_AGGREGATE_STATS_BODY]:
                stats_obj = OfpAggregateStatsCreator.create(
                    test_case_obj, dp,
                    ofproto, ofp_parser,
                    params[SCE_AGGREGATE_STATS_BODY][SCE_AGGREGATE_STATS])
                body = stats_obj
        kws[SCE_AGGREGATE_STATS_BODY] = body

        # create AggregateStatsReply.
        msg = ofp_parser.OFPAggregateStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_AGGREGATE

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
