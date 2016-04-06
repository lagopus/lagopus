import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_bucket_counter import SCE_BUCKET_STATS
from .ofp_bucket_counter import OfpBucketCounterCreator

# YAML:
# group_stats:
#   group_id: 0
#   ref_count: 0
#   packet_count: 0
#   byte_count: 0
#   bucket_stats:
#     - bucket_counter:
#         packet_count: 0
#         byte_count: 0

SCE_GROUP_STATS = "group_stats"


class OfpGroupStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # GroupStats.
        kws = copy.deepcopy(params)

        # bucket_stats.
        bucket_stats = []
        if SCE_BUCKET_STATS in params:
            bucket_stats = OfpBucketCounterCreator.create(test_case_obj,
                                                          dp, ofproto,
                                                          ofp_parser,
                                                          params[SCE_BUCKET_STATS])
        kws[SCE_BUCKET_STATS] = bucket_stats

        # create GroupStats.
        msg = ofp_parser.OFPGroupStats(**kws)

        msg._set_targets(["group_id", "ref_count",
                          "packet_count", "byte_count",
                          "bucket_stats"])

        return msg
