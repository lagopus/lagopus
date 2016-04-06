import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# get_async_request:

SCE_GET_ASYNC_REQUEST = "get_async_request"

@register_ofp_creators(SCE_GET_ASYNC_REQUEST)
class OfpGetAsyncRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # create GetAsyncRequest.
        msg = ofp_parser.OFPGetAsyncRequest(dp)

        return msg
