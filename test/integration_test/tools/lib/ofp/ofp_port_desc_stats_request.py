import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# port_desc_stats_request:
#   flags: 0

SCE_PORT_DESC_STATS_REQUEST = "port_desc_stats_request"


@register_ofp_creators(SCE_PORT_DESC_STATS_REQUEST)
class OfpPortDescStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # PortDescStatsRequest.
        kws = copy.deepcopy(params)

        # create PortDescStatsRequest.
        msg = ofp_parser.OFPPortDescStatsRequest(dp, **kws)

        return msg
