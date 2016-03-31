import os
import sys
import copy
import logging

from checker import *
from .ofp import register_ofp_creators
from .ofp import OfpBase
from .utils import get_attrs_without_len

# YAML:
# role_reply:
#   role: 0x0
#   generation_id: 0

SCE_ROLE_REPLY = "role_reply"


@register_ofp_creators(SCE_ROLE_REPLY)
class OfpRoleReplyCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # RoleReply.
        kws = copy.deepcopy(params)

        # create RoleReply.
        msg = ofp_parser.OFPRoleReply(dp, **kws)
        msg._set_targets(get_attrs_without_len(msg))

        return msg
