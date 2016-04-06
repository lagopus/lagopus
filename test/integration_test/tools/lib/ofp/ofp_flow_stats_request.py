import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_match import SCE_MATCH
from .ofp_match import OfpMatchCreator

# YAML:
# flow_stats_request:
#   cookie: 0
#   match:
#     in_port: 1
#     eth_dst: "ff:ff:ff:ff:ff:ff"

SCE_FLOW_STATS_REQUEST = "flow_stats_request"


@register_ofp_creators(SCE_FLOW_STATS_REQUEST)
class OfpFlowStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # FlowStatsRequest.
        kws = copy.deepcopy(params)

        # match.
        match = None
        if SCE_MATCH in params:
            match = OfpMatchCreator.create(test_case_obj,
                                           dp, ofproto,
                                           ofp_parser,
                                           params[SCE_MATCH])
        kws[SCE_MATCH] = match

        # create FlowStatsRequest.
        msg = ofp_parser.OFPFlowStatsRequest(dp, **kws)

        return msg
