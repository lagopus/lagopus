import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .utils import get_attrs_without_len

# YAML:
# barrier_reply:

SCE_BARRIER_REPLY = "barrier_reply"

@register_ofp_creators(SCE_BARRIER_REPLY)
class OfpBarrierReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # create BarrierReply.
        msg = ofp_parser.OFPBarrierReply(dp)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
