import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# meter_stats_request:
#   flags: 0
#   meter_id: 0

SCE_METER_STATS_REQUEST = "meter_stats_request"


@register_ofp_creators(SCE_METER_STATS_REQUEST)
class OfpMeterStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # MeterStatsRequest.
        kws = copy.deepcopy(params)

        # create MeterStatsRequest.
        msg = ofp_parser.OFPMeterStatsRequest(dp, **kws)

        return msg
