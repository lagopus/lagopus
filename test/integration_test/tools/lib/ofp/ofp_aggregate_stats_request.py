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
# aggregate_stats_request:
#   flags: 0
#   table_id: 0
#   out_port: 0
#   out_group: 0
#   cookie: 0
#   cookie_mask: 0
#   match:
#     in_port: 1
#     eth_dst: "ff:ff:ff:ff:ff:ff"

SCE_AGGREGATE_STATS_REQUEST = "aggregate_stats_request"


@register_ofp_creators(SCE_AGGREGATE_STATS_REQUEST)
class OfpAggregateStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # AggregateStatsRequest.
        kws = copy.deepcopy(params)

        # match.
        match = None
        if SCE_MATCH in params:
            match = OfpMatchCreator.create(test_case_obj,
                                           dp, ofproto,
                                           ofp_parser,
                                           params[SCE_MATCH])
        kws[SCE_MATCH] = match

        # create AggregateStatsRequest.
        msg = ofp_parser.OFPAggregateStatsRequest(dp, **kws)

        return msg
