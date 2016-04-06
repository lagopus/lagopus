import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_group_features_stats import SCE_GROUP_FEATURES_STATS
from .ofp_group_features_stats import OfpGroupFeaturesStatsCreator

# YAML:
# group_features_stats_reply:
#   flags: 0
#   body:
#     group_features_stats:
#       types: 0
#       capabilities: 0
#       max_groups:
#         - 0
#         - 0
#         - 0
#         - 0
#       actions:
#         - 0
#         - 0
#         - 0
#         - 0

SCE_GROUP_FEATURES_STATS_REPLY = "group_features_stats_reply"
SCE_GROUP_FEATURES_STATS_BODY = "body"


@register_ofp_creators(SCE_GROUP_FEATURES_STATS_REPLY)
class OfpGroupFeaturesStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # GroupFeaturesStatsReply.
        kws = copy.deepcopy(params)

        body = None
        if SCE_GROUP_FEATURES_STATS_BODY in params:
            if SCE_GROUP_FEATURES_STATS in params[SCE_GROUP_FEATURES_STATS_BODY]:
                features_stats = OfpGroupFeaturesStatsCreator.create(
                    test_case_obj, dp,
                    ofproto, ofp_parser,
                    params[SCE_GROUP_FEATURES_STATS_BODY][SCE_GROUP_FEATURES_STATS])
                body = features_stats
        kws[SCE_GROUP_FEATURES_STATS_BODY] = body

        # create GroupFeaturesStatsReply.
        msg = ofp_parser.OFPGroupFeaturesStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_GROUP_FEATURES

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
