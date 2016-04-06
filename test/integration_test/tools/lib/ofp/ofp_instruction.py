import os
import sys
import copy
import logging

from checker import *
from .utils import create_section_name
from .ofp import OfpBase
from .ofp_action import SCE_ACTIONS
from .ofp_action import OfpActionCreator
from .utils import get_attrs_without_len

# YAML:
#  - apply_actions:
#      actions:
#        - output:
#            port: 0

SCE_INSTRUCTIONS = "instructions"


class OfpInstructionCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # create section name: OFPInstruction...
        ins_classes = {}
        for inst in ofp_parser.OFPInstruction._INSTRUCTION_TYPES.values():
            key = create_section_name(inst.__name__, "OFPInstruction")
            ins_classes[key] = inst

        # create instructions.
        instructions = []
        for insts in params:
            for ins_type, ins_val in insts.items():
                if ins_type in ["apply_actions", "write_actions", "clear_actions"]:
                    # create actions.
                    actions = []
                    if SCE_ACTIONS in ins_val:
                        actions = OfpActionCreator.create(test_case_obj, dp,
                                                          ofproto, ofp_parser,
                                                          ins_val[SCE_ACTIONS])
                    ins_val[SCE_ACTIONS] = actions
                    ins_val["type_"] = getattr(
                        ofproto, "OFPIT_%s" % ins_type.upper())
                    # changed to OFPInstructionActions class.
                    ins_type = "actions"

                # create instructions.
                ins_obj = ins_classes[ins_type](**ins_val)
                ins_obj._set_targets(get_attrs_without_len(ins_obj))
                instructions.append(ins_obj)

        return instructions
