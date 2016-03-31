import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_bucket import SCE_BUCKETS
from .ofp_bucket import OfpBucketCreator

# YAML:
# group_mod:
#   command: 0
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

SCE_GROUP_MOD = "group_mod"


@register_ofp_creators(SCE_GROUP_MOD)
class OfpGroupModCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # GroupMod.
        kws = copy.deepcopy(params)

        # buckets.
        buckets = []
        if SCE_BUCKETS in params:
            buckets = OfpBucketCreator.create(test_case_obj,
                                              dp, ofproto,
                                              ofp_parser,
                                              params[SCE_BUCKETS])
        kws[SCE_BUCKETS] = buckets

        # create GroupMod.
        msg = ofp_parser.OFPGroupMod(dp, **kws)

        return msg
