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
# - port_stats:
#     port_no: 0
#     rx_packets: 0
#     tx_packets: 0
#     rx_bytes: 0
#     tx_bytes: 0
#     rx_dropped: 0
#     tx_dropped: 0
#     rx_errors: 0
#     tx_errors: 0
#     rx_frame_err: 0
#     rx_over_err: 0
#     rx_crc_err: 0
#     collisions: 0

SCE_PORT_STATS = "port_stats"


class OfpPortStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # PortStats.
        kws = copy.deepcopy(params)

        # create PortStats.
        msg = ofp_parser.OFPPortStats(**kws)

        msg._set_targets(["port_no", "rx_packets",
                          "tx_packets", "rx_bytes",
                          "tx_bytes", "rx_dropped",
                          "tx_dropped", "rx_errors",
                          "tx_errors", "rx_frame_err",
                          "rx_over_err", "rx_crc_err",
                          "collisions"])

        return msg
