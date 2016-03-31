import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_queue_prop import SCE_PROPERTIES
from .ofp_queue_prop import OfpQueuePropCreator
from .utils import get_attrs_without_len

# YAML:
# - packet_queue:
#     queue_id: 0
#     port: 0
#     properties:
#      - min_rate:
#          rate: 0

SCE_QUEUES = "queues"
SCE_PACKET_QUEUE = "packet_queue"


class OfpPacketQueueCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # PacketQueue.
        kws = copy.deepcopy(params)

        properties = []
        if SCE_PROPERTIES in params:
            properties = OfpQueuePropCreator.create(test_case_obj, dp,
                                                    ofproto, ofp_parser,
                                                    params[SCE_PROPERTIES])
        kws[SCE_PROPERTIES] = properties

        # create PacketQueue.
        msg = ofp_parser.OFPPacketQueue(**kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
