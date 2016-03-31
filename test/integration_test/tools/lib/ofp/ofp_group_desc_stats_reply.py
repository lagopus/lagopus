import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_group_desc_stats import SCE_GROUP_DESC_STATS
from .ofp_group_desc_stats import OfpGroupDescStatsCreator

# YAML:
# group_desc_stats_reply:
#   flags: 0
#   body:
#     - group_desc_stats:
#         type: 0
#         group_id: 0
#         buckets:
#           - bucket:
#               weight: 0
#               watch_port: 0
#               watch_group: 0
#               actions
#                 - output:
#                     port: 0

SCE_GROUP_DESC_STATS_REPLY = "group_desc_stats_reply"
SCE_GROUP_DESC_STATS_BODY = "body"


@register_ofp_creators(SCE_GROUP_DESC_STATS_REPLY)
class OfpGroupDescStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # GroupDescStatsReply.
        kws = copy.deepcopy(params)

        body = []
        if SCE_GROUP_DESC_STATS_BODY in params:
            for desc_stats in params[SCE_GROUP_DESC_STATS_BODY]:
                desc_stats_obj = OfpGroupDescStatsCreator.create(
                    test_case_obj, dp,
                    ofproto, ofp_parser,
                    desc_stats[SCE_GROUP_DESC_STATS])
                body.append(desc_stats_obj)
        kws[SCE_GROUP_DESC_STATS_BODY] = body

        # create GroupDescStatsReply.
        msg = ofp_parser.OFPGroupDescStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_GROUP_DESC

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
