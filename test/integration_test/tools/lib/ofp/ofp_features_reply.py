import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .utils import get_attrs_without_len

# YAML:
# features_reply:
#   datapath_id: 0x0
#   n_buffers: 0
#   n_tables: 0
#   auxiliary_id: 0
#   capabilities: 0x0

SCE_FEATURES_REPLY = "features_reply"

@register_ofp_creators(SCE_FEATURES_REPLY)
class OfpFeaturesReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # FeaturesReply.
        kws = copy.deepcopy(params)

        # create FeaturesReply.
        msg = ofp_parser.OFPSwitchFeatures(dp, **kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
