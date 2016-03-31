import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# queue_stats_request:
#   flags: 0
#   port_no: 0
#   queue_id: 0

SCE_QUEUE_STATS_REQUEST = "queue_stats_request"


@register_ofp_creators(SCE_QUEUE_STATS_REQUEST)
class OfpQueueStatsRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # QueueStatsRequest.
        kws = copy.deepcopy(params)

        # create QueueStatsRequest.
        msg = ofp_parser.OFPQueueStatsRequest(dp, **kws)

        return msg
