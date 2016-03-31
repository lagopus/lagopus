import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_action import SCE_ACTIONS
from .ofp_action import OfpActionCreator
from .utils import create_byte_data

# YAML:
# packet_out:
#   buffer_id: 0
#   in_port: 0
#   actions:
#     - output:
#        port: 0
#   data: "00 00" # Hex

SCE_PACKET_OUT = "packet_out"
SCE_PACKET_OUT_DATA = "data"

@register_ofp_creators(SCE_PACKET_OUT)
class OfpPacketOutCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # PacketOut.
        kws = copy.deepcopy(params)

        # actions.
        actions = []
        if SCE_ACTIONS in params:
            actions = OfpActionCreator.create(test_case_obj,
                                              dp, ofproto,
                                              ofp_parser,
                                              params[SCE_ACTIONS])
        kws[SCE_ACTIONS] = actions

        # data.
        data = None
        if SCE_PACKET_OUT_DATA in params:
            data = create_byte_data(params[SCE_PACKET_OUT_DATA])
        kws[SCE_PACKET_OUT_DATA] = data

        # create PacketOut.
        msg = ofp_parser.OFPPacketOut(dp, **kws)

        return msg
