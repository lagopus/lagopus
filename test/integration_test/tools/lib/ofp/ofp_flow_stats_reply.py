import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_flow_stats import SCE_FLOW_STATS
from .ofp_flow_stats import OfpFlowStatsCreator

# YAML:
# flow_stats_reply:
#   flags: 0
#   body:
#     - flow_stats:
#         cookie: 0
#         match:
#           in_port: 1
#           eth_dst: "ff:ff:ff:ff:ff:ff"
#           instructions:
#             - apply_actions:
#                 actions:
#                   - output:
#                       port: 0

SCE_FLOW_STATS_REPLY = "flow_stats_reply"
SCE_FLOW_STATS_BODY = "body"


@register_ofp_creators(SCE_FLOW_STATS_REPLY)
class OfpFlowStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # FlowStatsReply.
        kws = copy.deepcopy(params)

        body = []
        if SCE_FLOW_STATS_BODY in params:
            for stats in params[SCE_FLOW_STATS_BODY]:
                stats_obj = OfpFlowStatsCreator.create(test_case_obj, dp,
                                                       ofproto, ofp_parser,
                                                       stats[SCE_FLOW_STATS])
                body.append(stats_obj)
        kws[SCE_FLOW_STATS_BODY] = body

        # create FlowStatsReply.
        msg = ofp_parser.OFPFlowStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_FLOW

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
