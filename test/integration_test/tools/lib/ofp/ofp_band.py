import os
import sys
import copy
import logging

from checker import *
from .utils import create_section_name
from .ofp import OfpBase
from .utils import get_attrs_without_len

# YAML:
# - drop
#     rate: 0
#     burst_size: 0

SCE_BANDS = "bands"


class OfpBandCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # create section name: OFPBand...
        bd_classes = {}
        for bd in ofp_parser.OFPMeterBandHeader._METER_BAND.values():
            key = create_section_name(bd.__name__, "OFPMeterBand")
            bd_classes[key] = bd

        # create bands.
        bands = []
        for bd in params:
            for bd_type, bd_val in bd.items():
                bd_obj = bd_classes[bd_type](**bd_val)
                bd_obj._set_targets(get_attrs_without_len(bd_obj))
                bands.append(bd_obj)

        return bands
