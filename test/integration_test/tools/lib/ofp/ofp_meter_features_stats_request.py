import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# meter_features_stats_request:
#   flags: 0

SCE_METER_FEATURES_STATS_REQUEST = "meter_features_stats_request"


@register_ofp_creators(SCE_METER_FEATURES_STATS_REQUEST)
class OfpMeterFeaturesStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # MeterFeaturesStatsRequest.
        kws = copy.deepcopy(params)

        # create MeterFeaturesStatsRequest.
        msg = ofp_parser.OFPMeterFeaturesStatsRequest(dp, **kws)

        return msg
