import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# table_stats_request:
#   flags: 0

SCE_TABLE_STATS_REQUEST = "table_stats_request"


@register_ofp_creators(SCE_TABLE_STATS_REQUEST)
class OfpTableStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # TableStatsRequest.
        kws = copy.deepcopy(params)

        # create TableStatsRequest.
        msg = ofp_parser.OFPTableStatsRequest(dp, **kws)

        return msg
