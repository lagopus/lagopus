import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_band_stats import SCE_BAND_STATS
from .ofp_band_stats import OfpBandStatsCreator

# YAML:
# - meter_stats:
#     meter_id: 0
#     flow_count: 0
#     packet_in_count: 0
#     byte_in_count: 0
#     duration_sec: 0
#     duration_nsec: 0
#     band_stats:
#       - band_stats:
#           packet_band_count: 0
#           byte_band_count: 0

SCE_METER_STATS = "meter_stats"


class OfpMeterStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # MeterStats.
        kws = copy.deepcopy(params)

        # band_stats.
        band_stats = []
        if SCE_BAND_STATS in params:
            band_stats = OfpBandStatsCreator.create(test_case_obj,
                                                    dp, ofproto,
                                                    ofp_parser,
                                                    params[SCE_BAND_STATS])
        kws[SCE_BAND_STATS] = band_stats

        # create MeterStats.
        msg = ofp_parser.OFPMeterStats(**kws)

        msg._set_targets(["meter_id", "ref_count",
                          "packet_count", "byte_count",
                          "band_stats"])

        return msg
