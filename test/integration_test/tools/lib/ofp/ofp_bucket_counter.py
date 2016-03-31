import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .utils import get_attrs_without_len

# YAML:
#   bucket_stats:
#     - bucket_counter:
#         packet_count: 0
#         byte_count: 0

SCE_BUCKET_STATS = "bucket_stats"
SCE_BUCKET_COUNTER = "bucket_counter"


class OfpBucketCounterCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        bucket_counters = []

        for bc in params:
            if SCE_BUCKET_COUNTER in bc:
                bucket_counter = ofp_parser.OFPBucketCounter(**bc[SCE_BUCKET_COUNTER])
                bucket_counter._set_targets(get_attrs_without_len(bucket_counter))
                bucket_counters.append(bucket_counter)

        return bucket_counters
