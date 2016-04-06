import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# barrier_request:

SCE_BARRIER_REQUEST = "barrier_request"

@register_ofp_creators(SCE_BARRIER_REQUEST)
class OfpBarrierRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # create BarrierRequest.
        msg = ofp_parser.OFPBarrierRequest(dp)

        return msg
