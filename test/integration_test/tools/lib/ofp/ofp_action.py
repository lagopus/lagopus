import os
import sys
import copy
import logging

from checker import *
from .utils import create_section_name
from .ofp import OfpBase
from .utils import get_attrs_without_len

# YAML:
#  - output:
#      port: 0

SCE_ACTIONS = "actions"


class OfpActionCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # create section name: OFPAction...
        act_classes = {}
        for act in ofp_parser.OFPAction._ACTION_TYPES.values():
            key = create_section_name(act.__name__, "OFPAction")
            act_classes[key] = act

        # create actions.
        actions = []
        for act in params:
            for act_type, act_val in act.items():
                act_obj = act_classes[act_type](**act_val)
                act_obj._set_targets(get_attrs_without_len(act_obj))
                actions.append(act_obj)

        return actions
