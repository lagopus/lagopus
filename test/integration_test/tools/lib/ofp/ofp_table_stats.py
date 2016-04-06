import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# - table_stats:
#     table_id: 0
#     active_count: 0
#     lookup_count: 0
#     matched_count: 0

SCE_TABLE_STATS = "table_stats"


class OfpTableStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # TableStats.
        kws = copy.deepcopy(params)

        # create TableStats.
        msg = ofp_parser.OFPTableStats(**kws)

        msg._set_targets(["table_id", "active_count",
                          "lookup_count", "matched_count"])

        return msg
