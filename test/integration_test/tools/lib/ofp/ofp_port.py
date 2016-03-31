import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# - port:
#     port_no: 0
#     hw_addr: "00:00:00:00:00:00"
#     name: "xxx"
#     config: 0x0
#     state: 0x0
#     curr: 0x0
#     advertised: 0x0
#     supported: 0x0
#     peer: 0x0
#     curr_speed: 0
#     max_speed: 0

SCE_PORT = "port"


class OfpPortCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # Port.
        kws = copy.deepcopy(params)

        # create Port.
        msg = ofp_parser.OFPPort(**kws)

        msg._set_targets(["port_no", "hw_addr",
                          "name", "config",
                          "state", "curr",
                          "advertised", "supported",
                          "peer", "curr_speed",
                          "max_speed"])

        return msg
