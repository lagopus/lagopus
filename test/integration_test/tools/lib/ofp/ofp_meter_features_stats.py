import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_bucket import SCE_BUCKETS
from .ofp_bucket import OfpBucketCreator
from .utils import get_attrs_without_len

# YAML:
# - meter_features_stats:
#     max_meter: 0x0
#     band_types: 0x0
#     capabilities: 0x0
#     max_bands: 0x0
#     max_color: 0x0

SCE_METER_FEATURES_STATS = "meter_features_stats"


class OfpMeterFeaturesStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # MeterFeatures_Stats.
        kws = copy.deepcopy(params)

        # create MeterFeaturesStats.
        msg = ofp_parser.OFPMeterFeaturesStats(**kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
