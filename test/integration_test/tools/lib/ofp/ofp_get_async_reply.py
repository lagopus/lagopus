import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .utils import get_attrs_without_len

# YAML:
# get_async_reply:
#   packet_in_mask:
#     - 0x0
#     - 0x0
#   port_status_mask:
#     - 0x0
#     - 0x0
#   flow_removed_mask:
#     - 0x0
#     - 0x0

SCE_GET_ASYNC_REPLY = "get_async_reply"

@register_ofp_creators(SCE_GET_ASYNC_REPLY)
class OfpGetAsyncReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # GetAsyncReply.
        kws = copy.deepcopy(params)

        # create GetAsyncReply.
        msg = ofp_parser.OFPGetAsyncReply(dp, **kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
