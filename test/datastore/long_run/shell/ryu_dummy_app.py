from ryu.base.app_manager import RyuApp
from ryu.controller.handler import CONFIG_DISPATCHER
from ryu.controller.ofp_event import EventOFPSwitchFeatures
from ryu.controller.handler import set_ev_cls
from ryu.ofproto.ofproto_v1_3 import OFP_VERSION


class DummyApp(RyuApp):
    OFP_VERSIONS = [OFP_VERSION]

    def __init__(self, *args, **kwargs):
        super(DummyApp, self).__init__(*args, **kwargs)

    @set_ev_cls(EventOFPSwitchFeatures, CONFIG_DISPATCHER)
    def switch_features_handler(self, ev):
        pass
