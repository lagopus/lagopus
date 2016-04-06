import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_port import SCE_PORT
from .ofp_port import OfpPortCreator

# YAML:
# port_desc_stats_reply:
#   flags: 0
#   body:
#     - port:
#         port_no: 0
#         hw_addr: "00:00:00:00:00:00"
#         name: "xxx"
#         config: 0x0
#         state: 0x0
#         curr: 0x0
#         advertised: 0x0
#         supported: 0x0
#         peer: 0x0
#         curr_speed: 0
#         max_speed: 0

SCE_PORT_DESC_STATS_REPLY = "port_desc_stats_reply"
SCE_PORT_DESC_STATS_BODY = "body"


@register_ofp_creators(SCE_PORT_DESC_STATS_REPLY)
class OfpPortDescStatsReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # PortDescStatsReply.
        kws = copy.deepcopy(params)

        body = []
        if SCE_PORT_DESC_STATS_BODY in params:
            for desc_stats in params[SCE_PORT_DESC_STATS_BODY]:
                port_obj = OfpPortCreator.create(
                    test_case_obj, dp,
                    ofproto, ofp_parser,
                    desc_stats[SCE_PORT])
                body.append(port_obj)
        kws[SCE_PORT_DESC_STATS_BODY] = body

        # create PortDescStatsReply.
        msg = ofp_parser.OFPPortDescStatsReply(dp, **kws)
        msg.type = ofproto.OFPMP_PORT_DESC

        msg._set_targets(["version", "msg_type",
                          "body", "flags"])

        return msg
