import os
import sys
import logging

ofp_creators = {}


def register_ofp_creators(ofp_type_str):
    def _register_ofp_creators(cls):
        ofp_creators[ofp_type_str] = cls
        return cls

    return _register_ofp_creators


class OfpBase:

    @classmethod
    def create(cls, test_case_obj, dp, ofproto, ofp_parser, params):
        # override !!
        pass

import ofp_flow_mod
import ofp_flow_stats_request
import ofp_flow_stats_reply
import ofp_group_mod
import ofp_group_stats_request
import ofp_group_stats_reply
import ofp_group_desc_stats_request
import ofp_group_desc_stats_reply
