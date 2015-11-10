<!-- -*- mode: markdown ; coding: us-ascii-unix -*- -->

How to add/modify/delete/dump flows for Datastore
======================================

Add flow
---------------------------
### Usage

    flow <BRIDGE_NAME> add <MATCH_FIELDS> [<ACTONS>]

### Options
cf.) MATCH_FIELDS/ACTONS opts.

### Example

    flow bridge01 add in_port=1 apply_actions=output:2
    flow bridge01 add vlan_vid=1 dl_dst=ff:ff:ff:ff:ff:ff apply_actions=strip_vlan,output:2
    flow bridge01 add vlan_vid=2 dl_dst=ff:ff:ff:ff:ff:ff apply_actions=strip_vlan goto_table=1


Modify flow
---------------------------
### Usage

    flow <BRIDGE_NAME> mod [-strict] <MATCH_FIELDS> [<ACTONS>]


### Options
cf.) MATCH_FIELDS/ACTONS opts.

|Opts|Description|
|:--|:--|
|-strict|Strictly matching.|

### Example

    flow bridge01 mod in_port=1 apply_actions=output:3
    flow bridge01 mod vlan_vid=1 dl_dst=ff:ff:ff:ff:ff:ff apply_actions=strip_vlan,output:3
    flow bridge01 mod -strict vlan_vid=2 dl_dst=ff:ff:ff:ff:ff:ff apply_actions=strip_vlan goto_table=2

Delete flow
---------------------------
### Usage

    flow <BRIDGE_NAME> del [-strict] [<MATCH_FIELDS>]

### Options
cf.) MATCH_FIELDS opts.

|Opts|Description|
|:--|:--|
|-strict|Strictly matching.|

### Example

    flow bridge01 del in_port=1
    flow bridge01 del -strict vlan_vid=1 dl_dst=ff:ff:ff:ff:ff:ff

    # Delete all flows.
    flow bridge01 del

Dump flow
---------------------------
### Usage

    flow <BRIDGE_NAME> [-table-id <TABLE_ID>] [-with-stats]

### Options
|Opts|Description|
|:--|:--|
|-table-id|Specify a table ID.|
|-with-stats|With _stats_ (_packet\_count_, _byte\_count_).|

MATCH_FIELDS opts
---------------------------
|MATCH_FIELDS|Default|OFP 1.3.4|
|:--|:--|:--|
|in_port=in_port/table/normal/flood/all/controller/local/any/<0-4294967295>||OFPXMT_OFB_IN_PORT|
|in_phy_port=in_port/table/normal/flood/all/controller/local/<0-4294967295>||OFPXMT_OFB_IN_PHY_PORT|
|vlan_vid=<0-65535>[/<0-65535>]||OFPXMT_OFB_VLAN_VID|
|vlan_pcp=<0-7>||OFPXMT_OFB_VLAN_PCP|
|dl_src=xx:xx:xx:xx:xx:xx[/xx:xx:xx:xx:xx:xx]||OFPXMT_OFB_ETH_SRC|
|dl_dst=xx:xx:xx:xx:xx:xx[/xx:xx:xx:xx:xx:xx]||OFPXMT_OFB_ETH_DST|
|dl_type=arp/ip/ipv6/mpls/mplsmc/pbb/<0-65535>||OFPXMT_OFB_ETH_TYPE|
|nw_src=<__IP_V4__>[/<__IP_V4__>]||OFPXMT_OFB_IPV4_SRC|
|nw_dst=<__IP_V4__>[/<__IP_V4__>]||OFPXMT_OFB_IPV4_DST|
|nw_proto=<0-255>||OFPXMT_OFB_IP_PROTO|
|ip_dscp=<0-63>||OFPXMT_OFB_IP_DSCP|
|nw_ecn=<0-3>||OFPXMT_OFB_IP_ECN|
|tcp_src=<0-65535>||OFPXMT_OFB_TCP_SRC|
|tcp_dst=<0-65535>||OFPXMT_OFB_TCP_DST|
|udp_src=<0-65535>||OFPXMT_OFB_UDP_SRC|
|udp_dst=<0-65535>||OFPXMT_OFB_UDP_DST|
|sctp_src=<0-65535>||OFPXMT_OFB_SCTP_SRC|
|sctp_dst=<0-65535>||OFPXMT_OFB_SCTP_DST|
|icmp_type=<0-255>||OFPXMT_OFB_ICMPV4_TYPE|
|icmp_code=<0-255>||OFPXMT_OFB_ICMPV4_CODE|
|icmpv6_type=<0-255>||OFPXMT_OFB_ICMPV6_TYPE|
|icmpv6_code=<0-255>||OFPXMT_OFB_ICMPV6_CODE|
|pbb_isid=<0-16777215>[/<0-16777215>]||OFPXMT_OFB_PBB_ISID|
|ipv6_exthdr=<0-511>[/<0-511>]||OFPXMT_OFB_IPV6_EXTHDR|
|metadata=<0-18446744073709551615>[/18446744073709551615>]||OFPXMT_OFB_METADATA|
|arp_op=<0-65535>||OFPXMT_OFB_ARP_OP|
|arp_spa=<__IP_V4__>[/<__IP_V4__>]||OFPXMT_OFB_ARP_SPA|
|arp_tpa=<__IP_V4__>[/<__IP_V4__>]||OFPXMT_OFB_ARP_TPA|
|arp_sha=xx:xx:xx:xx:xx:xx[/xx:xx:xx:xx:xx:xx]||OFPXMT_OFB_ARP_SHA|
|arp_tha=xx:xx:xx:xx:xx:xx[/xx:xx:xx:xx:xx:xx]||OFPXMT_OFB_ARP_THA|
|ipv6_src=<__IP_V6__>[/<__IP_V6__>]||OFPXMT_OFB_IPV6_SRC|
|ipv6_dst=<__IP_V6__>[/<__IP_V6__>]||OFPXMT_OFB_IPV6_DST|
|ipv6_label=<0-1048575>[/<0-1048575>]||OFPXMT_OFB_IPV6_FLABEL|
|nd_target=<__IP_V6__>||OFPXMT_OFB_IPV6_ND_TARGET|
|nd_sll=xx:xx:xx:xx:xx:xx||OFPXMT_OFB_IPV6_ND_SLL|
|nd_tll=xx:xx:xx:xx:xx:xx||OFPXMT_OFB_IPV6_ND_TLL|
|mpls_bos=<0-1>||OFPXMT_OFP_MPLS_BOS|
|mpls_label=<0-1048575>||OFPXMT_OFB_MPLS_LABEL|
|mpls_tc=<0-7>||OFPXMT_OFB_MPLS_TC|
|tunnel_id=<0-18446744073709551615>[/<0-18446744073709551615>]||OFPXMT_OFB_TUNNEL_ID|
|table=all/<0-255>|0|ofp_flow_mod.table_id|
|cookie=<0-18446744073709551615>[/<0-18446744073709551615>]|0|ofp_flow_mod.cookie/cookie_mask|
|priority=<0-65535>|0|ofp_flow_mod.priority|
|idle_timeout=<0-65535>|0|ofp_flow_mod.idle_timeout|
|hard_timeout=<0-65535>|0|ofp_flow_mod.hard_timeout|
|send_flow_rem|false|ofp_flow_mod.flags|
|check_overlap|false|ofp_flow_mod.flags|
|out_port=in_port/table/normal/flood/all/controller/local/any/<0-4294967295>|any|ofp_flow_mod.out_port|
|out_group=all/any/<0-4294967295>|any|ofp_flow_mod.out_group|

ACTONS opts
---------------------------
|ACTIONS|ACTION|OFP 1.3.4|
|:--|:--|:--|
|apply_actions=[ACTION][,ACTION...]||OFPIT_APPLY_ACTIONS|
||output:in_port/table/normal/flood/all/controller/local/any/<0-4294967295>|OFPAT_OUTPUT|
||group:all/any/<0-4294967295>|OFPAT_GROUP|
||strip_vlan|OFPAT_POP_VLAN|
||push_vlan:<0-65535>|OFPAT_PUSH_VLAN|
||push_mpls:<0-65535>|OFPAT_PUSH_MPLS|
||pop_mpls:<0-65535>|OFPAT_POP_MPLS|
||set_queue:<0-4294967295>|OFPAT_SET_QUEUE|
||set_nw_ttl:<0-255>|OFPAT_SET_NW_TTL|
||dec_nw_ttl|OFPAT_DEC_NW_TTL|
||set_mpls_ttl:<0-255>|OFPAT_SET_MPLS_TTL|
||dec_mpls_ttl|OFPAT_DEC_MPLS_TTL|
||copy_ttl_out|OFPAT_COPY_TTL_OUT|
||copy_ttl_in|OFPAT_COPY_TTL_IN|
||push_pbb:<0-65535>|OFPAT_PUSH_PBB|
||pop_pbb|OFPAT_POP_PBB|
||vlan_vid:<0-4096>|OFPAT_SET_FIELD + OFPXMT_OFB_VLAN_VID|
||vlan_pcp:<0-7>|OFPAT_SET_FIELD + OFPXMT_OFB_VLAN_PCP|
||dl_src:xx:xx:xx:xx:xx:xx|OFPAT_SET_FIELD + OFPXMT_OFB_ETH_SRC|
||dl_dst:xx:xx:xx:xx:xx:xx|OFPAT_SET_FIELD + OFPXMT_OFB_ETH_DST|
||nw_src:<__IP_V4__>|OFPAT_SET_FIELD + OFPXMT_OFB_IPV4_SRC|
||nw_dst:<__IP_V4__>|OFPAT_SET_FIELD + OFPXMT_OFB_IPV4_DST|
||tcp_src:<0-4294967295>|OFPAT_SET_FIELD + OFPXMT_OFB_TCP_SRC|
||udp_src:<0-4294967295>|OFPAT_SET_FIELD + OFPXMT_OFB_UDP_SRC|
||sctp_src:<0-4294967295>|OFPAT_SET_FIELD + OFPXMT_OFB_SCTP_SRC|
||tcp_dst:<0-4294967295>|OFPAT_SET_FIELD + OFPXMT_OFB_TCP_DST|
||udp_dst:<0-4294967295>|OFPAT_SET_FIELD + OFPXMT_OFB_UDP_DST|
||sctp_dst:<0-4294967295>|OFPAT_SET_FIELD + OFPXMT_OFB_SCTP_DST|
||nw_ecn:<0-3>|OFPAT_SET_FIELD + OFPXMT_OFB_IP_ECN|
||ip_dscp:<0-63>|OFPAT_SET_FIELD + OFPXMT_OFB_IP_DSCP|
||dl_type:arp/ip/ipv6/mpls/mplsmc/pbb/<0-65535>|OFPAT_SET_FIELD + OFPXMT_OFB_ETH_TYPE|
||nw_proto:<0-255>|OFPAT_SET_FIELD + OFPXMT_OFB_IP_PROTO|
||icmp_type:<0-255>|OFPAT_SET_FIELD + OFPXMT_OFB_ICMPV4_TYPE|
||icmp_code:<0-255>|OFPAT_SET_FIELD + OFPXMT_OFB_ICMPV4_CODE|
||arp_op:<0-65535>|OFPAT_SET_FIELD + OFPXMT_OFB_ARP_OP|
||arp_spa:<__IP_V4__>|OFPAT_SET_FIELD + OFPXMT_OFB_ARP_SPA|
||arp_tpa:<__IP_V4__>|OFPAT_SET_FIELD + OFPXMT_OFB_ARP_TPA|
||arp_sha:xx:xx:xx:xx:xx:xx|OFPAT_SET_FIELD + OFPXMT_OFB_ARP_SHA|
||arp_tha:xx:xx:xx:xx:xx:xx|OFPAT_SET_FIELD + OFPXMT_OFB_ARP_THA|
||ipv6_src:<__IP_V6__>|OFPAT_SET_FIELD + OFPXMT_OFB_IPV6_SRC|
||ipv6_dst:<__IP_V6__>|OFPAT_SET_FIELD + OFPXMT_OFB_IPV6_DST|
||ipv6_label:<0-1048575>|OFPAT_SET_FIELD + OFPXMT_OFB_IPV6_FLABEL|
||icmpv6_type:<0-255>|OFPAT_SET_FIELD + OFPXMT_OFB_ICMPV6_TYPE|
||icmpv6_code:<0-255>|OFPAT_SET_FIELD + OFPXMT_OFB_ICMPV6_CODE|
||nd_target:<__IP_V6__>|OFPAT_SET_FIELD + OFPXMT_OFB_IPV6_ND_TARGET|
||nd_sll:xx:xx:xx:xx:xx:xx|OFPAT_SET_FIELD + OFPXMT_OFB_IPV6_ND_SLL|
||nd_tll:xx:xx:xx:xx:xx:xx|OFPAT_SET_FIELD + OFPXMT_OFB_IPV6_ND_TLL|
||mpls_label:<0-1048575>|OFPAT_SET_FIELD + OFPXMT_OFB_MPLS_LABEL|
||mpls_tc:<0-7>|OFPAT_SET_FIELD + OFPXMT_OFB_MPLS_TC|
||mpls_bos:<0-1>|OFPAT_SET_FIELD + OFPXMT_OFB_MPLS_BOS|
||pbb_isid:<0-16777215>|OFPAT_SET_FIELD + OFPXMT_OFB_PBB_ISID|
||tunnel_id:<0-18446744073709551615>|OFPAT_SET_FIELD + OFPXMT_OFB_TUNNEL_ID|
||ipv6_exthdr:<0-511>|OFPAT_SET_FIELD + OFPXMT_OFB_IPV6_EXTHDR|
||set_tunnel:<0-18446744073709551615>|OFPAT_SET_FIELD + OFPXMT_OFB_TUNNEL_ID|
||set_mpls_label:<0-1048575>|OFPAT_SET_FIELD + OFPXMT_OFB_MPLS_LABEL|
||set_mpls_tc:<0-7>|OFPAT_SET_FIELD + OFPXMT_OFB_MPLS_TC|
|write_actions=[ACTION][,ACTION...]||OFPIT_WRITE_ACTIONS|
||"Same as _apply\_actions_."|
|clear_actions||OFPIT_CLEAR_ACTIONS|
|write_metadata=<0-18446744073709551615>[/<0-18446744073709551615>]||OFPIT_WRITE_METADATA|
|meter=<0-4294967295>||OFPIT_METER|
|goto_table=<0-254>||OFPIT_GOTO_TABLE|
