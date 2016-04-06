import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_match import SCE_MATCH
from .ofp_match import OfpMatchCreator
from .ofp_instruction import SCE_INSTRUCTIONS
from .ofp_instruction import OfpInstructionCreator

# YAML:
# flow_stats:
#   cookie: 0
#   match:
#     in_port: 1
#     eth_dst: "ff:ff:ff:ff:ff:ff"
#     instructions:
#       - apply_actions:
#          actions:
#            - output:
#                port: 0

SCE_FLOW_STATS = "flow_stats"


class OfpFlowStatsCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # FlowStats.
        kws = copy.deepcopy(params)

        # match.
        match = None
        if SCE_MATCH in params:
            match = OfpMatchCreator.create(test_case_obj,
                                           dp, ofproto,
                                           ofp_parser,
                                           params[SCE_MATCH])
        kws[SCE_MATCH] = match

        # instructions.
        instructions = []
        if SCE_INSTRUCTIONS in params:
            instructions = OfpInstructionCreator.create(test_case_obj,
                                                        dp, ofproto,
                                                        ofp_parser,
                                                        params[SCE_INSTRUCTIONS])
        kws[SCE_INSTRUCTIONS] = instructions

        # create FlowStats.
        msg = ofp_parser.OFPFlowStats(**kws)

        msg._set_targets(["table_id", "priority",
                          "idle_timeout", "hard_timeout", "flags",
                          "cookie", "packet_count", "byte_count",
                          "match", "instructions"])

        return msg
