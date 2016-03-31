import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_match import SCE_MATCH
from .ofp_match import OfpMatchCreator

# YAML:
# group_desc_stats_request:
#   flags: 0

SCE_GROUP_DESC_STATS_REQUEST = "group_desc_stats_request"


@register_ofp_creators(SCE_GROUP_DESC_STATS_REQUEST)
class OfpGroupDescStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # GroupDescStatsRequest.
        kws = copy.deepcopy(params)

        # create GroupDescStatsRequest.
        msg = ofp_parser.OFPGroupDescStatsRequest(dp, **kws)

        return msg
