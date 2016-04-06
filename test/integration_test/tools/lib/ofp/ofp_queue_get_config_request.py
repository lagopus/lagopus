import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# queue_get_config_request:
#   port: 0

SCE_QUEUE_GET_CONFIG_REQUEST = "queue_get_config_request"


@register_ofp_creators(SCE_QUEUE_GET_CONFIG_REQUEST)
class OfpQueueGetConfigRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # QueueGetConfigRequest.
        kws = copy.deepcopy(params)

        # create QueueGetConfigRequest.
        msg = ofp_parser.OFPQueueGetConfigRequest(dp, **kws)

        return msg
