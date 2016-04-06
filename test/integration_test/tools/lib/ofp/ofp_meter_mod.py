import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_band import SCE_BANDS
from .ofp_band import OfpBandCreator

# YAML:
# meter_mod:
#   command: 0
#   flags: 0
#   meter_id: 0
#   bands:
#     - drop
#         rate: 0
#         burst_size: 0

SCE_METER_MOD = "meter_mod"


@register_ofp_creators(SCE_METER_MOD)
class OfpMeterModCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # MeterMod.
        kws = copy.deepcopy(params)

        # bands.
        bands = []
        if SCE_BANDS in params:
            bands = OfpBandCreator.create(test_case_obj,
                                          dp, ofproto,
                                          ofp_parser,
                                          params[SCE_BANDS])
        kws[SCE_BANDS] = bands

        # create MeterMod.
        msg = ofp_parser.OFPMeterMod(dp, **kws)

        return msg
