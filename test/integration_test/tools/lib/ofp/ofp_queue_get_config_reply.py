import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_packet_queue import SCE_QUEUES
from .ofp_packet_queue import SCE_PACKET_QUEUE
from .ofp_packet_queue import OfpPacketQueueCreator

# YAML:
# queue_get_config_reply:
#   port: 0
#   queues:
#     - packet_queue:
#         queue_id: 0
#         port: 0
#         properties:
#           - min_rate:
#               rate: 0

SCE_QUEUE_GET_CONFIG_REPLY = "queue_get_config_reply"


@register_ofp_creators(SCE_QUEUE_GET_CONFIG_REPLY)
class OfpQueueGetConfigReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # QueueGetConfigReply.
        kws = copy.deepcopy(params)

        queues = []
        if SCE_QUEUES in params:
            for q in params[SCE_QUEUES]:
                packet_queue =  OfpPacketQueueCreator.create(
                    test_case_obj, dp,
                    ofproto, ofp_parser,
                    q[SCE_PACKET_QUEUE])
                queues.append(packet_queue)
        kws[SCE_QUEUES] = queues

        # create QueueGetConfigReply.
        msg = ofp_parser.OFPQueueGetConfigReply(dp, **kws)

        return msg
