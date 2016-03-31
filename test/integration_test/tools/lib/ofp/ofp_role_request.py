import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase

# YAML:
# role_request:
#   role: 0x0
#   generation_id: 0

SCE_ROLE_REQUEST = "role_request"


@register_ofp_creators(SCE_ROLE_REQUEST)
class OfpRoleRequestCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # RoleRequest.
        kws = copy.deepcopy(params)

        # create RoleRequest.
        msg = ofp_parser.OFPRoleRequest(dp, **kws)

        return msg
