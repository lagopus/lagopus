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
# group_features_stats_request:
#   flags: 0
#   group_id: 0

SCE_GROUP_FEATURES_STATS_REQUEST = "group_features_stats_request"


@register_ofp_creators(SCE_GROUP_FEATURES_STATS_REQUEST)
class OfpGroupFeaturesStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # GroupFeaturesStatsRequest.
        kws = copy.deepcopy(params)

        # create GroupFeaturesStatsRequest.
        msg = ofp_parser.OFPGroupFeaturesStatsRequest(dp, **kws)

        return msg
