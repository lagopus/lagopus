import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# meter_config_stats_request:
#   flags: 0
#   meter_id: 0

SCE_METER_CONFIG_STATS_REQUEST = "meter_config_stats_request"


@register_ofp_creators(SCE_METER_CONFIG_STATS_REQUEST)
class OfpMeterConfigStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # MeterConfigStatsRequest.
        kws = copy.deepcopy(params)

        # create MeterConfigStatsRequest.
        msg = ofp_parser.OFPMeterConfigStatsRequest(dp, **kws)

        return msg
