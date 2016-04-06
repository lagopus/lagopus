import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_queue_stats import SCE_QUEUE_STATS
from .ofp_queue_stats import OfpQueueStatsCreator

# YAML:
# queue_stats_reply:
#   flags: 0
#   body:
#     - queue_stats:
#         port_no: 0
#         queue_id: 0
#         tx_bytes: 0
#         tx_packets: 0
#         tx_errors: 0
#         duration_sec: 0
#         duration_nsec: 0

SCE_QUEUE_STATS_REPLY = "queue_stats_reply"
SCE_QUEUE_STATS_BODY = "body"


@register_ofp_creators(SCE_QUEUE_STATS_REPLY)
class OfpQueueStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # QueueStatsReply.
        kws = copy.deepcopy(params)

        body = []
        if SCE_QUEUE_STATS_BODY in params:
            for stats in params[SCE_QUEUE_STATS_BODY]:
                stats_obj = OfpQueueStatsCreator.create(test_case_obj, dp,
                                                        ofproto, ofp_parser,
                                                        stats[SCE_QUEUE_STATS])
                body.append(stats_obj)
        kws[SCE_QUEUE_STATS_BODY] = body

        # create QueueStatsReply.
        msg = ofp_parser.OFPQueueStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_QUEUE

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
