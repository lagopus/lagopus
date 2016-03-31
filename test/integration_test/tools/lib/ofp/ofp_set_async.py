import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# set_async:
#   packet_in_mask:
#     - 0x0
#     - 0x0
#   port_status_mask:
#     - 0x0
#     - 0x0
#   flow_removed_mask:
#     - 0x0
#     - 0x0

SCE_SET_ASYNC = "set_async"

@register_ofp_creators(SCE_SET_ASYNC)
class OfpSetAsyncCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # SetAsync.
        kws = copy.deepcopy(params)

        # create SetAsync.
        msg = ofp_parser.OFPSetAsync(dp, **kws)

        return msg
