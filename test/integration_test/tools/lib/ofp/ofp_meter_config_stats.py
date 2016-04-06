import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_band import SCE_BANDS
from .ofp_band import OfpBandCreator
from .utils import get_attrs_without_len

# YAML:
#     - meter_config_stats:
#        flags: 0
#        meter_id: 0
#        bands:
#          - drop
#            rate: 0
#            burst_size: 0

SCE_METER_CONFIG_STATS = "meter_config_stats"


class OfpMeterConfigStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # MeterConfigStats.
        kws = copy.deepcopy(params)

        # bands.
        bands = []
        if SCE_BANDS in params:
            bands = OfpBandCreator.create(test_case_obj,
                                          dp, ofproto,
                                          ofp_parser,
                                          params[SCE_BANDS])
        kws[SCE_BANDS] = bands

        # create MeterConfigStats.
        msg = ofp_parser.OFPMeterConfigStats(**kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
