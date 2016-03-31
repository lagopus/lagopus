import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_group_stats import SCE_GROUP_STATS
from .ofp_group_stats import OfpGroupStatsCreator

# YAML:
# group_stats_reply:
#   flags: 0
#   body:
#     - group_stats:
#        group_id: 0
#        ref_count: 0
#        packet_count: 0
#        byte_count: 0
#        bucket_stats:
#          - bucket_counter:
#              packet_count: 0
#              byte_count: 0

SCE_GROUP_STATS_REPLY = "group_stats_reply"
SCE_GROUP_STATS_BODY = "body"


@register_ofp_creators(SCE_GROUP_STATS_REPLY)
class OfpGroupStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # GroupStatsReply.
        kws = copy.deepcopy(params)

        body = []
        if SCE_GROUP_STATS_BODY in params:
            for stats in params[SCE_GROUP_STATS_BODY]:
                stats_obj = OfpGroupStatsCreator.create(test_case_obj, dp,
                                                        ofproto, ofp_parser,
                                                        stats[SCE_GROUP_STATS])
                body.append(stats_obj)
        kws[SCE_GROUP_STATS_BODY] = body

        # create GroupStatsReply.
        msg = ofp_parser.OFPGroupStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_GROUP

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
