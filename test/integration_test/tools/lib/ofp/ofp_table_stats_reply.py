import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_table_stats import SCE_TABLE_STATS
from .ofp_table_stats import OfpTableStatsCreator

# YAML:
# table_stats_reply:
#   flags: 0
#   body:
#     - table_stats:
#         table_id: 0
#         active_count: 0
#         lookup_count: 0
#         matched_count: 0

SCE_TABLE_STATS_REPLY = "table_stats_reply"
SCE_TABLE_STATS_BODY = "body"


@register_ofp_creators(SCE_TABLE_STATS_REPLY)
class OfpTableStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # TableStatsReply.
        kws = copy.deepcopy(params)

        body = []
        if SCE_TABLE_STATS_BODY in params:
            for stats in params[SCE_TABLE_STATS_BODY]:
                stats_obj = OfpTableStatsCreator.create(test_case_obj, dp,
                                                        ofproto, ofp_parser,
                                                        stats[SCE_TABLE_STATS])
                body.append(stats_obj)
        kws[SCE_TABLE_STATS_BODY] = body

        # create TableStatsReply.
        msg = ofp_parser.OFPTableStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_TABLE

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
