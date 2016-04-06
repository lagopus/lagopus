import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# desc_stats_request:
#   flags: 0

SCE_DESC_STATS_REQUEST = "desc_stats_request"


@register_ofp_creators(SCE_DESC_STATS_REQUEST)
class OfpDescStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # DescStatsRequest.
        kws = copy.deepcopy(params)

        # create DescStatsRequest.
        msg = ofp_parser.OFPDescStatsRequest(dp, **kws)

        return msg
