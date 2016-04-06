import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_desc_stats import SCE_DESC_STATS
from .ofp_desc_stats import OfpDescStatsCreator

# YAML:
# desc_stats_reply:
#   flags: 0
#   body:
#     desc_stats:
#       mfr_desc: ""
#       hw_desc: ""
#       sw_desc: ""
#       serial_num: ""
#       dp_desc: ""

SCE_DESC_STATS_REPLY = "desc_stats_reply"
SCE_DESC_STATS_BODY = "body"


@register_ofp_creators(SCE_DESC_STATS_REPLY)
class OfpDescStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # DescStatsReply.
        kws = copy.deepcopy(params)

        body = None
        if SCE_DESC_STATS_BODY in params:
            if SCE_DESC_STATS in params[SCE_DESC_STATS_BODY]:
                stats_obj = OfpDescStatsCreator.create(
                    test_case_obj, dp,
                    ofproto, ofp_parser,
                    params[SCE_DESC_STATS_BODY][SCE_DESC_STATS])
                body = stats_obj
        kws[SCE_DESC_STATS_BODY] = body

        # create DescStatsReply.
        msg = ofp_parser.OFPDescStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_DESC

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
