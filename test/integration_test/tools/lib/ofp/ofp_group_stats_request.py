import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# group_stats_request:
#   flags: 0
#   group_id: 0

SCE_GROUP_STATS_REQUEST = "group_stats_request"


@register_ofp_creators(SCE_GROUP_STATS_REQUEST)
class OfpGroupStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # GroupStatsRequest.
        kws = copy.deepcopy(params)

        # create GroupStatsRequest.
        msg = ofp_parser.OFPGroupStatsRequest(dp, **kws)

        return msg
