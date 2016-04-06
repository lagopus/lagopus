import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .utils import get_attrs_without_len
from .utils import create_byte_data

# YAML:
# echo_reply:
#   data: "00 00" # Hex

SCE_ECHO_REPLY = "echo_reply"
SCE_ECHO_DATA = "data"

@register_ofp_creators(SCE_ECHO_REPLY)
class OfpEchoReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # EchoReply
        kws = copy.deepcopy(params)

        data = None
        if SCE_ECHO_DATA in params:
            data = create_byte_data(params[SCE_ECHO_DATA])

        kws[SCE_ECHO_DATA] = data

        # create EchoReply.
        msg = ofp_parser.OFPEchoReply(dp, **kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
