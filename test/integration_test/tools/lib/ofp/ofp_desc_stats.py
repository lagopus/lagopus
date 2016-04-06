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
# desc_stats:
#   mfr_desc: ""
#   hw_desc: ""
#   sw_desc: ""
#   serial_num: ""
#   dp_desc: ""

SCE_DESC_STATS = "desc_stats"


class OfpDescStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # DescStats.
        kws = copy.deepcopy(params)

        # create DescStats.
        msg = ofp_parser.OFPDescStats(**kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
