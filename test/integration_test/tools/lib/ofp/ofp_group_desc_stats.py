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
# group_desc_stats:
#   type: 0
#   group_id: 0
#   buckets:
#     - bucket:
#         weight: 0
#         watch_port: 0
#         watch_group: 0
#         actions
#           - output:
#               port: 0

SCE_GROUP_DESC_STATS = "group_desc_stats"


class OfpGroupDescStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # GroupDesc_Stats.
        kws = copy.deepcopy(params)

        # buckets.
        buckets = []
        if SCE_BUCKETS in params:
            buckets = OfpBucketCreator.create(test_case_obj,
                                              dp, ofproto,
                                              ofp_parser,
                                              params[SCE_BUCKETS])
        kws[SCE_BUCKETS] = buckets

        # create GroupDescStats.
        msg = ofp_parser.OFPGroupDescStats(**kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
