import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_band_stats import SCE_BAND_STATS
from .ofp_band_stats import OfpBandStatsCreator
from .utils import get_attrs_without_len

# YAML:
# aggregate_stats:
#   packet_count: 0
#   byte_count: 0
#   flow_count: 0

SCE_AGGREGATE_STATS = "aggregate_stats"


class OfpAggregateStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # AggregateStats.
        kws = copy.deepcopy(params)

        # create AggregateStats.
        msg = ofp_parser.OFPAggregateStats(**kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
