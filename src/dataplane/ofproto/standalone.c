/*
 * Copyright 2014 Nippon Telegraph and Telephone Corporation.
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


#include <inttypes.h>
#include <stdarg.h>
#include <sys/queue.h>

#include "openflow.h"
#include "lagopus/flowdb.h"
#include "lagopus/port.h"
#include "lagopus/dataplane.h"
#include "pktbuf.h"
#include "datapath_test_misc.h"
#include "datapath_test_misc_macros.h"

static void
add_apply_action(struct instruction *insn, uint16_t len, uint16_t type, ...) {
  va_list ap;
  struct action *action;
  uint16_t ofpat_len;
  uint8_t *p;
  int i;

  ofpat_len = ((len + 4 + 7) / 8) * 8; /* len + sizeof(ofp_action_header) must
                                          be padded to a multiple of 8. */
  action = calloc(1, sizeof(struct action) + ofpat_len - 4);
  action->ofpat.type = type;
  action->ofpat.len = ofpat_len;
  lagopus_set_action_function(action);
  p = ((uint8_t *)&action->ofpat) + 4;
  va_start(ap, type);
  for (i = 0; i < len; i++) {
    p[i] = (uint8_t)va_arg(ap, int);
  }
  va_end(ap);
  TAILQ_INSERT_TAIL(&insn->action_list, action, entry);
}

static void
register_flow_mpls_vlan(struct flowdb *flowdb, int i) {
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct instruction *insn;
  lagopus_result_t rv;
  struct ofp_error error;

  /* setup flow table. */
  flow_mod.table_id = 0;
  flow_mod.flags = 0;

  flow_mod.priority = 1;
  /* prepare match per flow entry. */
  TAILQ_INIT(&match_list);
  /* Port 1 */
  add_match(&match_list, 4, OFPXMT_OFB_IN_PORT << 1, 0, 0, 0, 1);
  /* Ether dst */
  add_match(&match_list, 6, OFPXMT_OFB_ETH_DST << 1, 0, 0, 0, 0, 0, 0);
  /* Ether type MPLS */
  add_match(&match_list, 2, OFPXMT_OFB_ETH_TYPE << 1, 0x88, 0x47);
  /* MPLS label */
  add_match(&match_list, 4, OFPXMT_OFB_MPLS_LABEL << 1,
            0, (i >> 16), (i >> 8) & 0xff, i & 0xff);

  TAILQ_INIT(&instruction_list);
  insn = (struct instruction *)calloc(1, sizeof(struct instruction));
  insn->ofpit.type = OFPIT_APPLY_ACTIONS;
  TAILQ_INIT(&insn->action_list);
  /* Pop MPLS */
  add_apply_action(insn, 2, OFPAT_POP_MPLS, 0x00, 0x08);
  /* Dec IPv4 TTL */
  add_apply_action(insn, 0, OFPAT_DEC_NW_TTL);
  /* Push VLAN */
  add_apply_action(insn, 2, OFPAT_PUSH_VLAN, 0x00, 0x81);
  /* Set VLAN-ID */
  add_apply_action(insn, 4+2, OFPAT_SET_FIELD,
                   0x80, 0x00, OFPXMT_OFB_VLAN_VID << 1, 2,
                   0x00, 0x01);
  /* Set Ether src */
  add_apply_action(insn, 4+6, OFPAT_SET_FIELD,
                   0x80, 0x00, OFPXMT_OFB_ETH_SRC << 1, 6,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
  /* Set Ether dst */
  add_apply_action(insn, 4+6, OFPAT_SET_FIELD,
                   0x80, 0x00, OFPXMT_OFB_ETH_DST << 1, 6,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
  /* Output to port 2 */
  add_apply_action(insn, 6, OFPAT_OUTPUT, 2, 0, 0, 0, 0, 0);
  TAILQ_INSERT_TAIL(&instruction_list, insn, entry);

  rv = flowdb_flow_add(flowdb, &flow_mod, &match_list,
                       &instruction_list, &error);
  if (rv != LAGOPUS_RESULT_OK) {
    printf("error got from flowdb_flow_add\n");
  }
}

static void
register_flow_vlan_mpls(struct flowdb *flowdb, int i) {
  struct ofp_flow_mod flow_mod;
  struct match_list match_list;
  struct instruction_list instruction_list;
  struct instruction *insn;
  lagopus_result_t rv;
  struct ofp_error error;
  int vid;

  /* setup flow table. */
  flow_mod.table_id = 0;
  flow_mod.flags = 0;

  flow_mod.priority = 1;
  /* prepare match per flow entry. */
  TAILQ_INIT(&match_list);
  /* Port 2 */
  add_match(&match_list, 4, OFPXMT_OFB_IN_PORT << 1, 0, 0, 0, 2);
  /* Ether dst */
  add_match(&match_list, 6, OFPXMT_OFB_ETH_DST << 1, 0, 0, 0, 0, 0, 0);
  /* Ether type IPv4 */
  add_match(&match_list, 2, OFPXMT_OFB_ETH_TYPE << 1, 0x08, 0x00);
  /* VLAN VID */
  vid = (i % 4094) + 1;
  add_match(&match_list, 2, OFPXMT_OFB_VLAN_VID << 1,
            (vid >> 8) | 0x10, vid & 0xff);
  /* IPv4 dst */
  add_match(&match_list, 8, (OFPXMT_OFB_IPV4_DST << 1) + 1,
            (i / 4094) & 0xff, 0, 0, 0, 0xff, 0xff, 0xff, 0x00);

  TAILQ_INIT(&instruction_list);
  insn = (struct instruction *)calloc(1, sizeof(struct instruction));
  insn->ofpit.type = OFPIT_APPLY_ACTIONS;
  TAILQ_INIT(&insn->action_list);
  /* Pop VLAN */
  add_apply_action(insn, 0, OFPAT_POP_VLAN);
  /* Dec IPv4 TTL */
  add_apply_action(insn, 0, OFPAT_DEC_NW_TTL);
  /* Push MPLS */
  add_apply_action(insn, 2, OFPAT_PUSH_MPLS, 0x47, 0x88);
  /* Set MPLS label */
  add_apply_action(insn, 4+4, OFPAT_SET_FIELD,
                   0x80, 0x00, OFPXMT_OFB_MPLS_LABEL << 1, 4,
                   0, (i >> 16), (i >> 8) & 0xff, i & 0xff);
  /* Set Ether src */
  add_apply_action(insn, 4+6, OFPAT_SET_FIELD,
                   0x80, 0x00, OFPXMT_OFB_ETH_SRC << 1, 6,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
  /* Set Ether dst */
  add_apply_action(insn, 4+6, OFPAT_SET_FIELD,
                   0x80, 0x00, OFPXMT_OFB_ETH_DST << 1, 6,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
  /* Output to port 1 */
  add_apply_action(insn, 6, OFPAT_OUTPUT, 1, 0, 0, 0, 0, 0);
  TAILQ_INSERT_TAIL(&instruction_list, insn, entry);

  rv = flowdb_flow_add(flowdb, &flow_mod, &match_list,
                       &instruction_list, &error);
  if (rv != LAGOPUS_RESULT_OK) {
    printf("error got from flowdb_flow_add\n");
  }
}

void
register_flow(struct flowdb *flowdb, int flow_num) {
  int i;

  /* 100,000 flow entries */
  for (i = 0; i < flow_num; i++) {
    register_flow_mpls_vlan(flowdb, i);
    register_flow_vlan_mpls(flowdb, i);
  }
}
