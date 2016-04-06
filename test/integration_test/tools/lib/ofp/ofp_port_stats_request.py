import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# port_stats_request:
#   flags: 0
#   port_no: 0

SCE_PORT_STATS_REQUEST = "port_stats_request"


@register_ofp_creators(SCE_PORT_STATS_REQUEST)
class OfpPortStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # PortStatsRequest.
        kws = copy.deepcopy(params)

        # create PortStatsRequest.
        msg = ofp_parser.OFPPortStatsRequest(dp, **kws)

        return msg
