import os
import sys
import copy
import logging

from checker import *
from .utils import create_section_name
from .ofp import OfpBase
from .ofp_action import SCE_ACTIONS
from .ofp_action import OfpActionCreator
from .utils import get_attrs_without_len

# YAML:
#  - bucket:
#      weight: 0
#      watch_port: 0
#      watch_group: 0
#      actions:
#        - output:
#            port: 0

SCE_BUCKETS = "buckets"
SCE_BUCKET = "bucket"


class OfpBucketCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # create buckets.
        buckets = []
        for bt in params:
            bt_val = bt[SCE_BUCKET]

            if SCE_ACTIONS in bt_val:
                actions = OfpActionCreator.create(test_case_obj, dp,
                                                  ofproto, ofp_parser,
                                                  bt_val[SCE_ACTIONS])
                bt_val[SCE_ACTIONS] = actions

            # create buckets.
            bucket_obj = ofp_parser.OFPBucket(**bt_val)
            bucket_obj._set_targets(get_attrs_without_len(bucket_obj))
            buckets.append(bucket_obj)

        return buckets
