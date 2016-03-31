import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_bucket import SCE_BUCKETS
from .ofp_bucket import OfpBucketCreator
from .utils import get_attrs_without_len

# YAML:
# group_features_stats:
#   types: 0
#   capabilities: 0
#   max_groups:
#     - 0
#     - 0
#     - 0
#     - 0
#   actions:
#     - 0
#     - 0
#     - 0
#     - 0

SCE_GROUP_FEATURES_STATS = "group_features_stats"


class OfpGroupFeaturesStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # GroupFeatures_Stats.
        kws = copy.deepcopy(params)

        # create GroupFeaturesStats.
        msg = ofp_parser.OFPGroupFeaturesStats(**kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
