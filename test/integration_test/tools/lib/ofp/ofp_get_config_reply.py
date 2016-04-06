import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .utils import get_attrs_without_len

# YAML:
# get_config_reply:
#   flags: 0x0
#   miss_send_len: 0

SCE_GET_CONFIG_REPLY = "get_config_reply"


@register_ofp_creators(SCE_GET_CONFIG_REPLY)
class OfpGetConfigReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # GetConfigReply.
        kws = copy.deepcopy(params)

        # create GetConfigReply.
        msg = ofp_parser.OFPGetConfigReply(dp, **kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
