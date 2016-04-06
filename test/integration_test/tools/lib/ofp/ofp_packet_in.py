import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_match import SCE_MATCH
from .ofp_match import OfpMatchCreator
from .utils import create_byte_data
from .utils import get_attrs_without_len

# YAML:
# packet_in:
#   buffer_id: 0
#   total_len: 0
#   reason: 0
#   table_id: 0
#   cookie: 0
#   match:
#     in_port: 1
#     eth_dst: "ff:ff:ff:ff:ff:ff"
#   data: "01 02" # Hex

SCE_PACKET_IN = "packet_in"
SCE_PACKET_IN_DATA = "data"

@register_ofp_creators(SCE_PACKET_IN)
class OfpPacketInCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # PacketIn.
        kws = copy.deepcopy(params)

        match = None
        if SCE_MATCH in params:
            match = OfpMatchCreator.create(test_case_obj,
                                           dp, ofproto,
                                           ofp_parser,
                                           params[SCE_MATCH])
        kws[SCE_MATCH] = match

        # data.
        data = None
        if SCE_PACKET_IN_DATA in params:
            data = create_byte_data(params[SCE_PACKET_IN_DATA])
        kws[SCE_PACKET_IN_DATA] = data

        # create PacketIn.
        msg = ofp_parser.OFPPacketIn(dp, **kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
