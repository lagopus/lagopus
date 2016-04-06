import os
import sys
import copy
import logging

from checker import *
from .utils import create_section_name
from .ofp import OfpBase
from .utils import get_attrs_without_len

# YAML:
#  - min_rate:
#      rate: 0

SCE_PROPERTIES = "properties"


class OfpQueuePropCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # create section name: OFPQueueProp...
        p_classes = {}
        for p in ofp_parser.OFPQueueProp._QUEUE_PROP_PROPERTIES.values():
            key = create_section_name(p.__name__, "OFPQueueProp")
            p_classes[key] = p

        # create properties.
        properties = []
        for p in params:
            for p_type, p_val in p.items():
                p_obj = p_classes[p_type](**p_val)
                p_obj._set_targets(get_attrs_without_len(p_obj))
                properties.append(p_obj)

        return properties
