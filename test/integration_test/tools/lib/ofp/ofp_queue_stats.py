import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_band_stats import SCE_BAND_STATS
from .ofp_band_stats import OfpBandStatsCreator

# YAML:
# - queue_stats:
#     port_no: 0
#     queue_id: 0
#     tx_bytes: 0
#     tx_packets: 0
#     tx_errors: 0
#     duration_sec: 0
#     duration_nsec: 0

SCE_QUEUE_STATS = "queue_stats"


class OfpQueueStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # QueueStats.
        kws = copy.deepcopy(params)

        # create QueueStats.
        msg = ofp_parser.OFPQueueStats(**kws)
        msg._set_targets(["port_no", "queue_id",
                          "tx_bytes", "tx_packets",
                          "tx_errors"])

        return msg
