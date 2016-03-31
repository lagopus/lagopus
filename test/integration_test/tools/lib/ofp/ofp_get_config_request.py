import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# get_config_request:

SCE_GET_CONFIG_REQUEST = "get_config_request"


@register_ofp_creators(SCE_GET_CONFIG_REQUEST)
class OfpGetConfigRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # create GetConfigRequest.
        msg = ofp_parser.OFPGetConfigRequest(dp)

        return msg
