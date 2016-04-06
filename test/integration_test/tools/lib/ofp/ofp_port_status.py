import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .ofp_port import SCE_PORT
from .ofp_port import OfpPortCreator
from .utils import get_attrs_without_len

# YAML:
# port_status:
#   reason: 0
#   desc:
#     port:
#       port_no: 0
#       hw_addr: "00:00:00:00:00:00"
#       name: "xxx"
#       config: 0x0
#       state: 0x0
#       curr: 0x0
#       advertised: 0x0
#       supported: 0x0
#       peer: 0x0
#       curr_speed: 0
#       max_speed: 0

SCE_PORT_STATUS = "port_status"
SCE_PORT_STATUS_DESC = "desc"

@register_ofp_creators(SCE_PORT_STATUS)
class OfpPortStatusCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # PortStatus.
        kws = copy.deepcopy(params)

        desc = None
        if SCE_PORT_STATUS_DESC in params:
            if SCE_PORT in params[SCE_PORT_STATUS_DESC]:
                port_obj = OfpPortCreator.create(
                    test_case_obj, dp,
                    ofproto, ofp_parser,
                    params[SCE_PORT_STATUS_DESC][SCE_PORT])
        kws[SCE_PORT_STATUS_DESC] = port_obj

        # create PortStatus.
        msg = ofp_parser.OFPPortStatus(dp, **kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
