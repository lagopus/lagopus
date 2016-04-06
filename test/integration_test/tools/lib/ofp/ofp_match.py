import os
import sys
import copy
import logging

from checker import *
from .ofp import OfpBase
from .utils import get_attrs_without_len

SCE_MATCH = "match"


class OfpMatchCreator(OfpBase):

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        match = ofp_parser.OFPMatch(**params)
        match._set_targets(get_attrs_without_len(match))

        return match
