import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_match import SCE_MATCH
from .ofp_match import OfpMatchCreator

# YAML:
# flow_removed:
#    cookie: 0
#    priority: 0
#    reason: 0
#    table_id: 0
#    idle_timeout: 0
#    hard_timeout: 0
#    packet_count: 0
#    byte_count: 0
#    match:
#      in_port: 1
#      eth_dst: "ff:ff:ff:ff:ff:ff"

SCE_FLOW_REMOVED = "flow_removed"
SCE_FLOW_STATS_BODY = "body"


@register_ofp_creators(SCE_FLOW_REMOVED)
class OfpFlowRemovedCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # FlowRemoved.
        kws = copy.deepcopy(params)

        # match.
        match = None
        if SCE_MATCH in params:
            match = OfpMatchCreator.create(test_case_obj,
                                           dp, ofproto,
                                           ofp_parser,
                                           params[SCE_MATCH])
        kws[SCE_MATCH] = match

        # create FlowRemoved.
        msg = ofp_parser.OFPFlowRemoved(dp, **kws)

        msg._set_targets(["cookie", "priority", "reason",
                          "table_id", "idle_timeout",
                          "hard_timeout", "packet_count",
                          "byte_count", "match"])

        return msg
