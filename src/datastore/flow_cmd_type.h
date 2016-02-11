/*
 * Copyright 2014-2016 Nippon Telegraph and Telephone Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file	flow_cmd_type.h
 */

#ifndef __FLOW_CMD_TYPE_H__
#define __FLOW_CMD_TYPE_H__

#define FLOW_UNKNOWN "unknown"
#define FLOW_NAME "name"
#define FLOW_TABLES "tables"
#define FLOW_FLOWS "flows"
#define FLOW_ACTIONS "actions"
#define FLOW_STATS "stats"

/* cmd type. */
enum flow_cmd_type {
  FLOW_CMD_TYPE_MATCH = 0,
  FLOW_CMD_TYPE_INSTRUCTION,
  FLOW_CMD_TYPE_ACTION,
  FLOW_CMD_TYPE_ACTION_SET_FIELD,
  FLOW_CMD_TYPE_FLOW_FIELD,
};

/* port. */
enum flow_port {
  FLOW_PORT_IN_PORT = 0,
  FLOW_PORT_TABLE,
  FLOW_PORT_NORMAL,
  FLOW_PORT_FLOOD,
  FLOW_PORT_ALL,
  FLOW_PORT_CONTROLLER,
  FLOW_PORT_LOCAL,
  FLOW_PORT_ANY,

  FLOW_PORT_MAX,
};

/* port str. */
static const char *const flow_port_strs[FLOW_PORT_MAX] = {
  "in_port",                  /* FLOW_PORT_IN_PORT */
  "table",                    /* FLOW_PORT_TABLE */
  "normal",                   /* FLOW_PORT_NORMAL */
  "flood",                    /* FLOW_PORT_FLOOD */
  "all",                      /* FLOW_PORT_ALL */
  "controller",               /* FLOW_PORT_CONTROLLER */
  "local",                    /* FLOW_PORT_LOCAL */
  "any",                      /* FLOW_PORT_ANY */
};

/* dl type. */
enum flow_dl_type {
  FLOW_DL_TYPE_IP = 0,
  FLOW_DL_TYPE_ARP,
  FLOW_DL_TYPE_VLAN,
  FLOW_DL_TYPE_IPV6,
  FLOW_DL_TYPE_MPLS,
  FLOW_DL_TYPE_MPLS_MCAST,
  FLOW_DL_TYPE_PBB,

  FLOW_DL_TYPE_MAX,
};

/* dl type str. */
static const char *const flow_dl_type_strs[FLOW_DL_TYPE_MAX] = {
  "ip",                     /* FLOW_DL_TYPE_IP */
  "arp",                    /* FLOW_DL_TYPE_ARP */
  "vlan",                   /* FLOW_DL_TYPE_VLAN */
  "ipv6",                   /* FLOW_DL_TYPE_IPV6 */
  "mpls",                   /* FLOW_DL_TYPE_MPLS */
  "mpls_mcast",             /* FLOW_DL_TYPE_MPLS_MCAST */
  "pbb",                    /* FLOW_DL_TYPE_PBB */
};

/* group. */
enum flow_group {
  FLOW_GROUP_ALL = 0,
  FLOW_GROUP_ANY,

  FLOW_GROUP_MAX,
};

/* group str. */
static const char *const flow_group_strs[FLOW_GROUP_MAX] = {
  "all",                    /* FLOW_GROUP_ALL */
  "any",                    /* FLOW_GROUP_ANY */
};

/* table. */
enum flow_table {
  FLOW_TABLE_ALL = 0,

  FLOW_TABLE_MAX,
};

/* table str. */
static const char *const flow_table_strs[FLOW_TABLE_MAX] = {
  "all",                    /* FLOW_TABLE_ALL */
};

/* match field. */
enum flow_match_field {
  FLOW_MATCH_FIELD_IN_PORT = 0,
  FLOW_MATCH_FIELD_IN_PHY_PORT,
  FLOW_MATCH_FIELD_METADATA,
  FLOW_MATCH_FIELD_DL_DST,
  FLOW_MATCH_FIELD_DL_SRC,
  FLOW_MATCH_FIELD_DL_TYPE,
  FLOW_MATCH_FIELD_VLAN_VID,
  FLOW_MATCH_FIELD_VLAN_PCP,
  FLOW_MATCH_FIELD_IP_DSCP,
  FLOW_MATCH_FIELD_NW_ECN,
  FLOW_MATCH_FIELD_NW_PROTO,
  FLOW_MATCH_FIELD_NW_SRC,
  FLOW_MATCH_FIELD_NW_DST,
  FLOW_MATCH_FIELD_TCP_SRC,
  FLOW_MATCH_FIELD_TCP_DST,
  FLOW_MATCH_FIELD_UDP_SRC,
  FLOW_MATCH_FIELD_UDP_DST,
  FLOW_MATCH_FIELD_SCTP_SRC,
  FLOW_MATCH_FIELD_SCTP_DST,
  FLOW_MATCH_FIELD_ICMP_TYPE,
  FLOW_MATCH_FIELD_ICMP_CODE,
  FLOW_MATCH_FIELD_ARP_OP,
  FLOW_MATCH_FIELD_ARP_SPA,
  FLOW_MATCH_FIELD_ARP_TPA,
  FLOW_MATCH_FIELD_ARP_SHA,
  FLOW_MATCH_FIELD_ARP_THA,
  FLOW_MATCH_FIELD_IPV6_SRC,
  FLOW_MATCH_FIELD_IPV6_DST,
  FLOW_MATCH_FIELD_IPV6_LABEL,
  FLOW_MATCH_FIELD_ICMPV6_TYPE,
  FLOW_MATCH_FIELD_ICMPV6_CODE,
  FLOW_MATCH_FIELD_ND_TARGET,
  FLOW_MATCH_FIELD_ND_SLL,
  FLOW_MATCH_FIELD_ND_TLL,
  FLOW_MATCH_FIELD_MPLS_LABEL,
  FLOW_MATCH_FIELD_MPLS_TC,
  FLOW_MATCH_FIELD_MPLS_BOS,
  FLOW_MATCH_FIELD_PBB_ISID,
  FLOW_MATCH_FIELD_TUNNEL_ID,
  FLOW_MATCH_FIELD_IPV6_EXTHDR,
  FLOW_MATCH_FIELD_PBB_UCA,
  FLOW_MATCH_FIELD_PACKET_TYPE,
  FLOW_MATCH_FIELD_GRE_FLAGS,
  FLOW_MATCH_FIELD_GRE_VER,
  FLOW_MATCH_FIELD_GRE_PROTOCOL,
  FLOW_MATCH_FIELD_GRE_KEY,
  FLOW_MATCH_FIELD_GRE_SEQNUM,
  FLOW_MATCH_FIELD_LISP_FLAGS,
  FLOW_MATCH_FIELD_LISP_NONCE,
  FLOW_MATCH_FIELD_LISP_ID,
  FLOW_MATCH_FIELD_VXLAN_FLAGS,
  FLOW_MATCH_FIELD_VXLAN_VNI,
  FLOW_MATCH_FIELD_MPLS_DATA_FIRST_NIBBLE,
  FLOW_MATCH_FIELD_MPLS_ACH_VERSION,
  FLOW_MATCH_FIELD_MPLS_ACH_CHANNEL,
  FLOW_MATCH_FIELD_MPLS_PW_METADATA,
  FLOW_MATCH_FIELD_MPLS_CW_FLAGS,
  FLOW_MATCH_FIELD_MPLS_CW_FRAG,
  FLOW_MATCH_FIELD_MPLS_CW_LEN,
  FLOW_MATCH_FIELD_MPLS_CW_SEQ_NUM,

  FLOW_MATCH_FIELD_MAX,
};

/* match field str. */
static const char *const flow_match_field_strs[FLOW_MATCH_FIELD_MAX] = {
  "in_port",                      /* FLOW_MATCH_FIELD_IN_PORT */
  "in_phy_port",                  /* FLOW_MATCH_FIELD_IN_PHY_PORT */
  "metadata",                     /* FLOW_MATCH_FIELD_METADATA */
  "dl_dst",                       /* FLOW_MATCH_FIELD_DL_DST */
  "dl_src",                       /* FLOW_MATCH_FIELD_DL_SRC */
  "dl_type",                      /* FLOW_MATCH_FIELD_DL_TYPE */
  "vlan_vid",                     /* FLOW_MATCH_FIELD_VLAN_VID */
  "vlan_pcp",                     /* FLOW_MATCH_FIELD_VLAN_PCP */
  "ip_dscp",                      /* FLOW_MATCH_FIELD_IP_DSCP */
  "nw_ecn",                       /* FLOW_MATCH_FIELD_NW_ECN */
  "nw_proto",                     /* FLOW_MATCH_FIELD_NW_PROTO */
  "nw_src",                       /* FLOW_MATCH_FIELD_NW_SRC */
  "nw_dst",                       /* FLOW_MATCH_FIELD_NW_DST */
  "tcp_src",                      /* FLOW_MATCH_FIELD_TCP_SRC */
  "tcp_dst",                      /* FLOW_MATCH_FIELD_TCP_DST */
  "udp_src",                      /* FLOW_MATCH_FIELD_UDP_SRC */
  "udp_dst",                      /* FLOW_MATCH_FIELD_UDP_DST */
  "sctp_src",                     /* FLOW_MATCH_FIELD_SCTP_SRC */
  "sctp_dst",                     /* FLOW_MATCH_FIELD_SCTP_DST */
  "icmp_type",                    /* FLOW_MATCH_FIELD_ICMP_TYPE */
  "icmp_code",                    /* FLOW_MATCH_FIELD_ICMP_CODE */
  "arp_op",                       /* FLOW_MATCH_FIELD_ARP_OP */
  "arp_spa",                      /* FLOW_MATCH_FIELD_ARP_SPA */
  "arp_tpa",                      /* FLOW_MATCH_FIELD_ARP_TPA */
  "arp_sha",                      /* FLOW_MATCH_FIELD_ARP_SHA */
  "arp_tha",                      /* FLOW_MATCH_FIELD_ARP_THA */
  "ipv6_src",                     /* FLOW_MATCH_FIELD_IPV6_SRC */
  "ipv6_dst",                     /* FLOW_MATCH_FIELD_IPV6_DST */
  "ipv6_label",                   /* FLOW_MATCH_FIELD_IPV6_LABEL */
  "icmpv6_type",                  /* FLOW_MATCH_FIELD_ICMPV6_TYPE */
  "icmpv6_code",                  /* FLOW_MATCH_FIELD_ICMPV6_CODE */
  "nd_target",                    /* FLOW_MATCH_FIELD_ND_TARGET */
  "nd_sll",                       /* FLOW_MATCH_FIELD_ND_SLL */
  "nd_tll",                       /* FLOW_MATCH_FIELD_ND_TLL */
  "mpls_label",                   /* FLOW_MATCH_FIELD_MPLS_LABEL */
  "mpls_tc",                      /* FLOW_MATCH_FIELD_MPLS_TC */
  "mpls_bos",                     /* FLOW_MATCH_FIELD_MPLS_BOS */
  "pbb_isid",                     /* FLOW_MATCH_FIELD_PBB_ISID */
  "tunnel_id",                    /* FLOW_MATCH_FIELD_TUNNEL_ID */
  "ipv6_exthdr",                  /* FLOW_MATCH_FIELD_IPV6_EXTHDR */
  "pbb_uca",                      /* FLOW_MATCH_FIELD_PBB_UCA */
  "packet_type",                  /* FLOW_MATCH_FIELD_PACKET_TYPE */
  "gre_flags",                    /* FLOW_MATCH_FIELD_GRE_FLAGS */
  "gre_ver",                      /* FLOW_MATCH_FIELD_GRE_VER */
  "gre_protocol",                 /* FLOW_MATCH_FIELD_GRE_PROTOCOL */
  "gre_key",                      /* FLOW_MATCH_FIELD_GRE_KEY */
  "gre_seqnum",                   /* FLOW_MATCH_FIELD_GRE_SEQNUM */
  "lisp_flags",                   /* FLOW_MATCH_FIELD_LISP_FLAGS */
  "lisp_nonce",                   /* FLOW_MATCH_FIELD_LISP_NONCE */
  "lisp_id",                      /* FLOW_MATCH_FIELD_LISP_ID */
  "vxlan_flags",                  /* FLOW_MATCH_FIELD_VXLAN_FLAGS */
  "vxlan_vni",                    /* FLOW_MATCH_FIELD_VXLAN_VNI */
  "mpls_data_first_nibble",       /* FLOW_MATCH_FIELD_MPLS_DATA_FIRST_NIBBLE */
  "mpls_ach_version",             /* FLOW_MATCH_FIELD_MPLS_ACH_VERSION */
  "mpls_ach_channel",             /* FLOW_MATCH_FIELD_MPLS_ACH_CHANNEL */
  "mpls_pw_metadata",             /* FLOW_MATCH_FIELD_MPLS_PW_METADATA */
  "mpls_cw_flags",                /* FLOW_MATCH_FIELD_MPLS_CW_FLAGS */
  "mpls_cw_frag",                 /* FLOW_MATCH_FIELD_MPLS_CW_FRAG */
  "mpls_cw_len",                  /* FLOW_MATCH_FIELD_MPLS_CW_LEN */
  "mpls_cw_seq_num",              /* FLOW_MATCH_FIELD_MPLS_CW_SEQ_NUM */
};

/* action. */
enum flow_action {
  FLOW_ACTION_OUTPUT = 0,
  FLOW_ACTION_COPY_TTL_OUT,
  FLOW_ACTION_COPY_TTL_IN,
  FLOW_ACTION_SET_MPLS_TTL,
  FLOW_ACTION_DEC_MPLS_TTL,
  FLOW_ACTION_PUSH_VLAN,
  FLOW_ACTION_STRIP_VLAN,
  FLOW_ACTION_PUSH_MPLS,
  FLOW_ACTION_POP_MPLS,
  FLOW_ACTION_SET_QUEUE,
  FLOW_ACTION_GROUP,
  FLOW_ACTION_SET_NW_TTL,
  FLOW_ACTION_DEC_NW_TTL,
  FLOW_ACTION_SET_FIELD,
  FLOW_ACTION_PUSH_PBB,
  FLOW_ACTION_POP_PBB,
  FLOW_ACTION_ENCAP,
  FLOW_ACTION_DECAP,
  FLOW_ACTION_EXPERIMENTER,

  FLOW_ACTION_MAX,
};

/* action str. */
static const char *const flow_action_strs[FLOW_ACTION_MAX] = {
  "output",                      /* FLOW_ACTION_OUTPUT */
  "copy_ttl_out",                /* FLOW_ACTION_COPY_TTL_OUT */
  "copy_ttl_in",                 /* FLOW_ACTION_COPY_TTL_IN */
  "set_mpls_ttl",                /* FLOW_ACTION_SET_MPLS_TTL */
  "dec_mpls_ttl",                /* FLOW_ACTION_DEC_MPLS_TTL */
  "push_vlan",                   /* FLOW_ACTION_PUSH_VLAN */
  "strip_vlan",                  /* FLOW_ACTION_STRIP_VLAN */
  "push_mpls",                   /* FLOW_ACTION_PUSH_MPLS */
  "pop_mpls",                    /* FLOW_ACTION_POP_MPLS */
  "set_queue",                   /* FLOW_ACTION_SET_QUEUE */
  "group",                       /* FLOW_ACTION_GROUP */
  "set_nw_ttl",                  /* FLOW_ACTION_SET_NW_TTL */
  "dec_nw_ttl",                  /* FLOW_ACTION_DEC_NW_TTL */
  "set_field",                   /* FLOW_ACTION_SET_FIELD */
  "push_pbb",                    /* FLOW_ACTION_PUSH_PBB */
  "pop_pbb",                     /* FLOW_ACTION_POP_PBB */
  "encap",                       /* FLOW_ACTION_ENCAP */
  "decap",                       /* FLOW_ACTION_DECAP */
  "experimenter",                /* FLOW_ACTION_EXPERIMENTER */
};

/* instruction. */
enum flow_instruction {
  FLOW_INSTRUCTION_GOTO_TABLE = 0,
  FLOW_INSTRUCTION_WRITE_METADATA,
  FLOW_INSTRUCTION_WRITE_ACTIONS,
  FLOW_INSTRUCTION_APPLY_ACTIONS,
  FLOW_INSTRUCTION_CLEAR_ACTIONS,
  FLOW_INSTRUCTION_METER,
  FLOW_INSTRUCTION_EXPERIMENTER,

  FLOW_INSTRUCTION_MAX,
};

/* instruction str. */
static const char *const flow_instruction_strs[FLOW_INSTRUCTION_MAX] = {
  "goto_table",                 /* FLOW_INSTRUCTION_GOTO_TABLE */
  "write_metadata",             /* FLOW_INSTRUCTION_WRITE_METADATA */
  "write_actions",              /* FLOW_INSTRUCTION_WRITE_ACTIONS */
  "apply_actions",              /* FLOW_INSTRUCTION_APPLY_ACTIONS */
  "clear_actions",              /* FLOW_INSTRUCTION_CLEAR_ACTIONS */
  "meter",                      /* FLOW_INSTRUCTION_METER */
  "experimenter",               /* FLOW_INSTRUCTION_EXPERIMENTER */
};

/* flow field. */
enum flow_field {
  FLOW_FIELD_COOKIE = 0,
  FLOW_FIELD_TABLE,
  FLOW_FIELD_IDLE_TIMEOUT,
  FLOW_FIELD_HARD_TIMEOUT,
  FLOW_FIELD_PRIORITY,
  FLOW_FIELD_OUT_PORT,
  FLOW_FIELD_OUT_GROUP,
  FLOW_FIELD_SEND_FLOW_REM,
  FLOW_FIELD_CHECK_OVERLAP,

  FLOW_FIELD_MAX,
};

/* flow mod field str. */
static const char *const flow_field_strs[FLOW_FIELD_MAX] = {
  "cookie",                    /* FLOW_FIELD_COOKIE */
  "table",                     /* FLOW_FIELD_TABLE */
  "idle_timeout",              /* FLOW_FIELD_IDLE_TIMEOUT */
  "hard_timeout",              /* FLOW_FIELD_HARD_TIMEOUT */
  "priority",                  /* FLOW_FIELD_PRIORITY */
  "out_port",                  /* FLOW_FIELD_OUT_PORT */
  "out_group",                 /* FLOW_FIELD_OUT_GROUP */
  "send_flow_rem",             /* FLOW_FIELD_SEND_FLOW_REM */
  "check_overlap",             /* FLOW_FIELD_CHECK_OVERLAP */
};

/* flow mod option. */
enum flow_mod_opt {
  FLOW_MOD_OPT_STRICT = 0,

  FLOW_MOD_OPT_MAX,
};

/* flow mod option str. */
static const char *const flow_mod_opt_strs[FLOW_MOD_OPT_MAX] = {
  "-strict",                   /* FLOW_MOD_OPT_STRICT */
};

/* flow field. */
enum flow_stat {
  FLOW_STAT_PACKET_COUNT = 0,
  FLOW_STAT_BYTE_COUNT,

  FLOW_STAT_MAX,
};

/* flow mod field str. */
static const char *const flow_stat_strs[FLOW_STAT_MAX] = {
  "packet_count",              /* FLOW_STAT_PACKET_COUNT */
  "byte_count",                /* FLOW_STAT_BYTE_COUNT */
};

/* action encap. */
enum action_encap {
  FLOW_ACTION_ENCAP_PACKET_TYPE = 0,
  FLOW_ACTION_ENCAP_PROPS,

  FLOW_ACTION_ENCAP_MAX,
};

/* action encap str. */
static const char *const action_encap_strs[FLOW_ACTION_ENCAP_MAX] = {
  "packet_type",               /* FLOW_ACTION_ENCAP_PACKET_TYPE */
  "properties",                /* FLOW_ACTION_ENCAP_PROPS */
};

/* action decap. */
enum action_decap {
  FLOW_ACTION_DECAP_CUR_PKT_TYPE = 0,
  FLOW_ACTION_DECAP_NEW_PKT_TYPE,
  FLOW_ACTION_DECAP_PROPS,

  FLOW_ACTION_DECAP_MAX,
};

/* action decap str. */
static const char *const action_decap_strs[FLOW_ACTION_DECAP_MAX] = {
  "cur_packet_type",           /* FLOW_ACTION_DECAP_CUR_PKT_TYPE */
  "new_packet_type",           /* FLOW_ACTION_DECAP_NEW_PKT_TYPE */
  "properties",                /* FLOW_ACTION_DECAP_PROPS */
};

/* ed prop. */
enum encap_decup_prop {
  FLOW_ACTION_ED_PROP_PORT_NAME = 0,

  FLOW_ACTION_ED_PROP_MAX,
};

/* ed prop str. */
static const char *const ed_prop_strs[FLOW_ACTION_ED_PROP_MAX] = {
  "port_name",                 /* FLOW_ACTION_ED_PROP_PORT_NAME */
};

/* ed prop port name. */
enum encap_decup_prop_portname {
  FLOW_ACTION_ED_PROP_PORT_NAME_PORT_FLAGS = 0,
  FLOW_ACTION_ED_PROP_PORT_NAME_NAME,

  FLOW_ACTION_ED_PROP_PORT_NAME_MAX,
};

/* ed prop str. */
static const char *const
ed_prop_portname_strs[FLOW_ACTION_ED_PROP_PORT_NAME_MAX] = {
  "port_flags",               /* FLOW_ACTION_ED_PROP_PORT_NAME_PORT_FLAGS */
  "name",                     /* FLOW_ACTION_ED_PROP_PORT_NAME_NAME */
};

#endif /* __FLOW_CMD_TYPE_H__ */
