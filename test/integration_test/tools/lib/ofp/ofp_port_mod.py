import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# port_mod:
#   port_no: 0
#   hw_addr: "00:00:00:00:00:00"
#   config: 0x0
#   mask: 0x0
#   advertise: 0x0

SCE_PORT_MOD = "port_mod"


@register_ofp_creators(SCE_PORT_MOD)
class OfpPortModCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # PortMod.
        kws = copy.deepcopy(params)

        # create PortMod.
        msg = ofp_parser.OFPPortMod(dp, **kws)

        return msg
