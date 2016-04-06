import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# features_request:

SCE_FEATURES_REQUEST = "features_request"

@register_ofp_creators(SCE_FEATURES_REQUEST)
class OfpFeaturesRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # create FeaturesRequest.
        msg = ofp_parser.OFPFeaturesRequest(dp)

        return msg
