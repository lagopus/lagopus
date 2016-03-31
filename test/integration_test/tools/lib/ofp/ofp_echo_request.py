import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .utils import create_byte_data

# YAML:
# echo_request:
#   data: "00 00" # Hex

SCE_ECHO_REQUEST = "echo_request"
SCE_ECHO_DATA = "data"

@register_ofp_creators(SCE_ECHO_REQUEST)
class OfpEchoRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # EchoRequest
        kws = copy.deepcopy(params)

        data = None
        if SCE_ECHO_DATA in params:
            data = create_byte_data(params[SCE_ECHO_DATA])

        kws[SCE_ECHO_DATA] = data

        # create EchoRequest.
        msg = ofp_parser.OFPEchoRequest(dp, **kws)

        return msg
