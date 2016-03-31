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

from . import ofp_flow_mod
from . import ofp_flow_stats_request
from . import ofp_flow_stats_reply
from . import ofp_flow_removed
from . import ofp_group_mod
from . import ofp_group_stats_request
from . import ofp_group_stats_reply
from . import ofp_group_desc_stats_request
from . import ofp_group_desc_stats_reply
from . import ofp_group_features_stats_request
from . import ofp_group_features_stats_reply
from . import ofp_meter_mod
from . import ofp_meter_stats_request
from . import ofp_meter_stats_reply
from . import ofp_meter_config_stats_request
from . import ofp_meter_config_stats_reply
from . import ofp_meter_features_stats_request
from . import ofp_meter_features_stats_reply
from . import ofp_port_mod
from . import ofp_port_stats_request
from . import ofp_port_stats_reply
from . import ofp_port_desc_stats_request
from . import ofp_port_desc_stats_reply
from . import ofp_port_status
from . import ofp_barrier_request
from . import ofp_barrier_reply
from . import ofp_features_request
from . import ofp_features_reply
from . import ofp_echo_request
from . import ofp_echo_reply
from . import ofp_table_stats_request
from . import ofp_table_stats_reply
from . import ofp_get_config_request
from . import ofp_get_config_reply
from . import ofp_set_config
from . import ofp_role_request
from . import ofp_role_reply
from . import ofp_get_async_request
from . import ofp_get_async_reply
from . import ofp_set_async
from . import ofp_queue_get_config_request
from . import ofp_queue_get_config_reply
from . import ofp_queue_stats_request
from . import ofp_queue_stats_reply
from . import ofp_aggregate_stats_request
from . import ofp_aggregate_stats_reply
from . import ofp_desc_stats_request
from . import ofp_desc_stats_reply
from . import ofp_packet_out
from . import ofp_packet_in
