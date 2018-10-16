#ifndef  ELEM_C_H
#define  ELEM_C_H

#include <stdlib.h>
#include <sys/queue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include <lagopus_apis.h>
#include <lagopus/dp_apis.h>
#include <lagopus/flowdb.h>

#include "City.h"
#include "pktbuf.h"
#include "packet.h"
#include "misc.h"

#define FieldDA 1
#define FieldSP 2
#define FieldDP 3
#define FieldProto 4

#define low_dim 0
#define high_dim 1

#define POINT_SIZE_BITS 32

//From THTable.h
#define OXM_FIELD_TYPE(field) ((field) >> 1)
#define OXM_FIELD_HAS_MASK(field) (((field) & 1) != 0)
#define OXM_MATCH_VALUE_LEN(match) \
  ((match)->oxm_length >> ((match)->oxm_field & 1))


//#define NUM_CLASSIFY_FIELDS 5

typedef uint32_t memory;
typedef uint32_t point;
typedef point* packet;



/*	
FIELD TYPES
======================
  OFPXMT_OFB_IN_PORT        = 0, 
  OFPXMT_OFB_IN_PHY_PORT    = 1,
  OFPXMT_OFB_METADATA       = 2,
  OFPXMT_OFB_ETH_DST        = 3,
  OFPXMT_OFB_ETH_SRC        = 4,
  OFPXMT_OFB_ETH_TYPE       = 5,
  OFPXMT_OFB_VLAN_VID       = 6,
  OFPXMT_OFB_VLAN_PCP       = 7,
  OFPXMT_OFB_IP_DSCP        = 8,
  OFPXMT_OFB_IP_ECN         = 9,
  OFPXMT_OFB_IP_PROTO       = 10,
  OFPXMT_OFB_IPV4_SRC       = 11,
  OFPXMT_OFB_IPV4_DST       = 12,
  OFPXMT_OFB_TCP_SRC        = 13,
  OFPXMT_OFB_TCP_DST        = 14,
  OFPXMT_OFB_UDP_SRC        = 15,
  OFPXMT_OFB_UDP_DST        = 16,
  OFPXMT_OFB_SCTP_SRC       = 17,
  OFPXMT_OFB_SCTP_DST       = 18,
  OFPXMT_OFB_ICMPV4_TYPE    = 19,
  OFPXMT_OFB_ICMPV4_CODE    = 20,
  OFPXMT_OFB_ARP_OP         = 21,
  OFPXMT_OFB_ARP_SPA        = 22,
  OFPXMT_OFB_ARP_TPA        = 23,
  OFPXMT_OFB_ARP_SHA        = 24,
  OFPXMT_OFB_ARP_THA        = 25,
  OFPXMT_OFB_IPV6_SRC       = 26,
  OFPXMT_OFB_IPV6_DST       = 27,
  OFPXMT_OFB_IPV6_FLABEL    = 28,
  OFPXMT_OFB_ICMPV6_TYPE    = 29,
  OFPXMT_OFB_ICMPV6_CODE    = 30,
  OFPXMT_OFB_IPV6_ND_TARGET = 31,
  OFPXMT_OFB_IPV6_ND_SLL    = 32,
  OFPXMT_OFB_IPV6_ND_TLL    = 33,
  OFPXMT_OFB_MPLS_LABEL     = 34,
  OFPXMT_OFB_MPLS_TC        = 35,
  OFPXMT_OFB_MPLS_BOS       = 36,
  OFPXMT_OFB_PBB_ISID       = 37,
  OFPXMT_OFB_TUNNEL_ID      = 38,
  OFPXMT_OFB_IPV6_EXTHDR    = 39,*/
//  OFPXMT_OFB_PBB_UCA                = 41,/* PBB UCA header field. */
//  OFPXMT_OFB_PACKET_TYPE            = 42, /* packet type value. */
// OFPXMT_OFB_GRE_FLAGS              = 43, /* GRE Flags */
//  OFPXMT_OFB_GRE_VER                = 44, /* GRE Version */
//  OFPXMT_OFB_GRE_PROTOCOL           = 45, /* GRE Protocol */
//  OFPXMT_OFB_GRE_KEY                = 46, /* GRE Key */
//  OFPXMT_OFB_GRE_SEQNUM             = 47, /* GRE Sequence number */
//  OFPXMT_OFB_LISP_FLAGS             = 48, /* LISP Flags */
//  OFPXMT_OFB_LISP_NONCE             = 49, /* LiSP NONCE Echo field */
//  OFPXMT_OFB_LISP_ID                = 50, /* LISP  ID */
//  OFPXMT_OFB_VXLAN_FLAGS            = 51, /* VXLAN Flags */
//  OFPXMT_OFB_VXLAN_VNI              = 52, /* VXLAN ID */
//  OFPXMT_OFB_MPLS_DATA_FIRST_NIBBLE = 53, /* MPLS First nibble */
//  OFPXMT_OFB_MPLS_ACH_VERSION       = 54, /* MPLS Associated Channel version 0 or 1 */
//  OFPXMT_OFB_MPLS_ACH_CHANNEL       = 55, /* MPLS Associated Channel number */
//  OFPXMT_OFB_MPLS_PW_METADATA       = 56, /* MPLS PW Metadata */
//  OFPXMT_OFB_MPLS_CW_FLAGS          = 57, /* MPLS PW Control Word Flags */
//  OFPXMT_OFB_MPLS_CW_FRAG           = 58, /* MPLS PW CW Fragment */
//  OFPXMT_OFB_MPLS_CW_LEN            = 59, /* MPLS PW CW Length */
//  OFPXMT_OFB_MPLS_CW_SEQ_NUM        = 60, /* MPLS PW CW Sequence number */
/*  OFPXMT_OFB_GTPU_FLAGS             = 61,
  OFPXMT_OFB_GTPU_VER               = 62,
  OFPXMT_OFB_GTPU_MSGTYPE           = 63,
  OFPXMT_OFB_GTPU_TEID              = 64,
  OFPXMT_OFB_GTPU_EXTN_HDR          = 65,
  OFPXMT_OFB_GTPU_EXTN_UDP_PORT     = 66,
  OFPXMT_OFB_GTPU_EXTN_SCI          = 67*/

//
// Allows for fields of rule to be specified by range or value/mask pair
// Makes rules work with classifiers that require contiguous rules
// as well as those that allow for discontiguous rules
//
typedef union rule_specifier{
	point range[2];
	struct{
		point value;
		point mask;
	}
} rule_specifier;

typedef struct rule
{
	long long priority;
	int dim;
	int id;
	int tag;
	unsigned *prefix_length;
	union rule_specifier *fields; 
	struct flow *master;
	bool contiguous;
	bool marked_delete;
} rule;

typedef struct interval{
	point a, b;
	int id;
	int weight;
} interval;

typedef struct end_point {
	double val;
	bool is_right_end;
	int id;
} end_point;

rule* rule_init(int dimension);
rule* rule_init_c(const struct flow *flow, uint8_t *fields, const int NUM_FIELDS);//Converts a flow to a rule considering only the fields specified
bool rule_matches_l_packet(struct rule *rule, const struct lagopus_packet* p);//Returns true if the lagopus packet is in the desired range for each match field
void pbuf_fill(const struct lagopus_packet *lp, uint8_t *fields, const int NUM_FIELDS, packet p);//Extracts the given fields from the packet and stores them in the packet, p
long long local_hash_flow(struct flow *flow);

//Takes a pointer to a pointer of a rule and deallocates all dynamic memory associated with the rule.
static inline void rule_destroy(struct rule *rule){
	free(rule->fields);
	free(rule->prefix_length);
	free(rule);
}

//Returns true if the extracted fields from the flow match the extracted fields from the packet
//This method does not check whether all of the match fields in the flow match the corresponding fields in the packet 
static inline bool contiguous_rule_matches_packet(struct rule *rule, const packet p){
	for (int i = 0; i < rule->dim; ++i) {
		if (p[i] < rule->fields[i].range[low_dim] || p[i] > rule->fields[i].range[high_dim]) return false;
	}
	return true;
}

//Returns true if the extracted fields from the flow match the extracted fields from the packet
//This method does not check whether all of the match fields in the flow match the corresponding fields in the packet 
static inline bool discontiguous_rule_matches_packet(struct rule *rule, const packet p){
	//printf("\n");
	for (int i = 0; i < rule->dim; ++i) {
		if ((p[i] & rule->fields[i].mask) != rule->fields[i].value){  return false; }
		//printf("%d,",i);
	}
	return true;
}

static inline bool intersects_rules(const struct rule *rule1, const struct rule *rule2){
	for (int i = 0; i < rule1->dim; ++i) {
		if (rule1->fields[i].range[high_dim] < rule2->fields[i].range[low_dim] || rule1->fields[i].range[low_dim] > rule2->fields[i].range[high_dim]) return false;
	}
	return true;
}

static inline void print_rule(struct rule *rule){
	for (int i = 0; i < rule->dim; ++i) {
		printf("%u:%u ", rule->fields[i].range[low_dim], rule->fields[i].range[high_dim]);
	}
	printf("\n%lld\n",rule->priority);
	printf("\n");
}

static inline void print_packet(packet p, const int NUM_FIELDS){
	for (int i = 0; i < NUM_FIELDS; ++i) {
		printf("%u ", p[i]);
	}
	printf("\n");
}

static int inline interval_less_than(const void *l, const void *r){
	interval *lhs = (interval *)l;
	interval *rhs = (interval *)r;
	if (lhs->a != rhs->a) {
		return lhs->a - rhs->a;
	} else return lhs->b - rhs->b;
}
static bool inline interval_equals(const struct interval* lhs, const struct interval* rhs){
	return lhs->a == rhs->a && lhs->b == rhs->b;
}

static void inline build_interval(unsigned a, unsigned b, int id, interval *interval){
	interval->a = a;
	interval->b = b;
	interval->id = id;
}

static inline int end_point_less_than(const void *lhs, const void *rhs){
	end_point *elhs = (end_point *)lhs;
	end_point *erhs = (end_point *)rhs;
	return elhs->val - erhs->val;
}

static inline int compare_rules(const void* r1, const void *r2){
	rule *rx = (rule *)r1;
	rule *ry = (rule *)r2;
	return rx->priority >= ry->priority ? -1 : 1;
}

static inline void sort_rules(struct rule **rules, const int NUM_RULES) {
	qsort(rules, NUM_RULES, sizeof(struct rule *), compare_rules);
}

static inline packet pbuf_init(const int PKT_SIZE){
	return (packet)safe_malloc(sizeof(point) * PKT_SIZE);
}

static inline void pbuf_destroy(packet p){
	free(p);
}
#endif
