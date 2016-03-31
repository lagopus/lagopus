import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .utils import get_attrs_without_len

# YAML:
# band_stats:
#   - band_stats:
#       packet_band_count: 0
#       byte_band_count: 0


SCE_BAND_STATS = "band_stats"


class OfpBandStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        band_stats = []

        for bc in params:
            if SCE_BAND_STATS in bc:
                band_obj = ofp_parser.OFPMeterBandStats(**bc[SCE_BAND_STATS])
                band_obj._set_targets(get_attrs_without_len(band_obj))
                band_stats.append(band_obj)

        return band_stats
