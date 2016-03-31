import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# set_config:
#   flags: 0x0
#   miss_send_len: 0

SCE_SET_CONFIG = "set_config"


@register_ofp_creators(SCE_SET_CONFIG)
class OfpSetConfigCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # SetConfig.
        kws = copy.deepcopy(params)

        # create SetConfig.
        msg = ofp_parser.OFPSetConfig(dp, **kws)

        return msg
