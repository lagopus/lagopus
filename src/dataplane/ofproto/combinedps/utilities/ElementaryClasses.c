#include "ElementaryClasses.h"
#include "../City.h"
#include <stdio.h>
#include <string.h>

//Start of code from thtable

struct match_idx_temp {
	int base;
	int off;
	int size;
	uint8_t mask[16];
	int shift;
};

/*
 * pkt value: &base[idx])[off], sizeof(member), masked, right shifted
 * match value: oxm_value, OXM_MATCH_VALUE_LEN
 */
#define MAKE_IDX(base, type, member)        \
  (base), offsetof(struct type, member), sizeof(((struct type *)0)->member)

static const struct match_idx_temp match_idx_temp[]  = {
	{ MAKE_IDX(OOB_BASE, oob_data, in_port), {0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(OOB_BASE, oob_data, in_phy_port), {0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(OOB_BASE, oob_data, metadata), {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(ETH_BASE, ether_header, ether_dhost), {0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(ETH_BASE, ether_header, ether_shost), {0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(OOB_BASE, oob_data, ether_type), {0xff,0xff}, 0 },
	{ MAKE_IDX(OOB_BASE, oob_data, vlan_tci), {0x1f,0xff}, 0 },
	{ MAKE_IDX(OOB_BASE, oob_data, vlan_tci), {0xe0,00}, 13 },
	{ MAKE_IDX(L3_BASE, ip, ip_tos), {0xfc}, 3 },
	{ MAKE_IDX(L3_BASE, ip, ip_tos), {0x03}, 0 },
	{ IPPROTO_BASE, 0, 1, {0xff}, 0 },
	{ MAKE_IDX(L3_BASE, ip, ip_src), {0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(L3_BASE, ip, ip_dst), {0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(L4_BASE, tcphdr, th_sport), {0xff,0xff}, 0 },
	{ MAKE_IDX(L4_BASE, tcphdr, th_dport), {0xff,0xff}, 0 },
	{ MAKE_IDX(L4_BASE, udphdr, uh_sport), {0xff,0xff}, 0 },
	{ MAKE_IDX(L4_BASE, udphdr, uh_dport), {0xff,0xff}, 0 },
	{ MAKE_IDX(L4_BASE, tcphdr, th_sport), {0xff,0xff}, 0 },
	{ MAKE_IDX(L4_BASE, tcphdr, th_dport), {0xff,0xff}, 0 },
	{ MAKE_IDX(L4_BASE, icmp, icmp_type), {0xff}, 0 },
	{ MAKE_IDX(L4_BASE, icmp, icmp_code), {0xff}, 0 },
	{ MAKE_IDX(L3_BASE, ether_arp, arp_op), {0xff,0xff}, 0 },
	{ MAKE_IDX(L3_BASE, ether_arp, arp_spa), {0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(L3_BASE, ether_arp, arp_tpa), {0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(L3_BASE, ether_arp, arp_sha), {0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(L3_BASE, ether_arp, arp_tha), {0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(L3_BASE, ip6_hdr, ip6_src), {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(L3_BASE, ip6_hdr, ip6_dst), {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(L3_BASE, ip6_hdr, ip6_flow), {0x00,0x0f,0xff,0xff}, 0 },
	{ MAKE_IDX(L4_BASE, icmp6_hdr, icmp6_type), {0xff}, 0 },
	{ MAKE_IDX(L4_BASE, icmp6_hdr, icmp6_code), {0xff}, 0 },
	{ L4_BASE, 0, 0, 0, 0},
	{ NDSLL_BASE, 0, 2, {0xff,0xff}, 0 },
	{ NDTLL_BASE, 0, 2, {0xff,0xff}, 0 },
	{ MPLS_BASE, 0, 3, {0xff,0xff,0xf0}, 12 },
	{ MPLS_BASE, 2, 1, {0x01}, 0 },
	{ MPLS_BASE, 3, 1, {0xff}, 0 },
	{ MAKE_IDX(PBB_BASE, pbb_hdr, i_sid), {0x00,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(OOB2_BASE, oob2_data, tunnel_id), {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, 0 },
	{ MAKE_IDX(OOB2_BASE, oob2_data, ipv6_exthdr), {0xff,0xff}, 0 }
};
//End of Code from THTable

rule* rule_init(int dimension){ 
	rule *r = (rule *)safe_malloc(sizeof(rule));	
	
	r->dim = dimension;
	r->tag = -1;
	r->id = -1;
	
	r->prefix_length = (unsigned *)calloc(dimension, sizeof(unsigned));
	
	r->fields = (union rule_specifier *)calloc(dimension, sizeof(union rule_specifier));
	
	r->contiguous = true;
	r->marked_delete = false;
	r->priority = 0; 
	r->master = NULL;
	return r;
}

//Converts a flow to a rule considering only the fields specified 
rule* rule_init_c(const struct flow *flow, uint8_t *fields, const int NUM_FIELDS){

	unsigned max_length[NUM_FIELDS];

	struct rule *r = (rule*)safe_malloc(sizeof(struct rule));	
	r->dim = NUM_FIELDS;
	r->tag = -1;
	r->id = -1;
	r->marked_delete = false;

	struct match *match = NULL;
	int count = 0, i = 0;
	uint64_t key = 0;

	r->prefix_length = (unsigned *)malloc(NUM_FIELDS * sizeof(unsigned));//Allocate memory for each prefix length
	
	r->fields = (union rule_specifier *)calloc(NUM_FIELDS, sizeof(union rule_specifier));

	r->priority = flow->priority;
	r->master = flow;//Must keep track of this to match against a packet

	if(fields == NULL){
		//printf("NO FIELDS\n");
		return r; //No reason to extract fields, none to extract
	}

	r->contiguous = true;

	//Loop through all matching fields and extract the desired fields
	TAILQ_FOREACH(match, &flow->match_list, entry){			

		if(count == NUM_FIELDS)
			break;
								
		for(i = 0; i < NUM_FIELDS; ++i){

			//If the field is a match field
			if(match->oxm_field == fields[i]){

				++count;
				//r->prefix_length[i] = match->oxm_length; // Too simple, need to adjust

				//If the field has a bitmask, convert to range
				if(match->oxm_field & 1){
					max_length[i] = 8 * match->oxm_length / 2;
					uint32_t value = 0;
					uint32_t mask = 0;

					memcpy(&value, match->oxm_value, match->oxm_length/2);
					memcpy(&mask, &match->oxm_value[match->oxm_length/2], match->oxm_length/2);

					value = OS_NTOHL(value);
					mask = OS_NTOHL(mask);
					//printf("%u:%x, ",value & mask,mask);
					
					r->prefix_length[i] = __builtin_popcount(mask); // Calculate specified bits
					
					r->fields[i].value = value & mask; // Ensure value is masked value (error check)
					r->fields[i].mask = mask;
					
					//Perform contiguity test
					//=======================
					//If contiguous, mask is one less that a power of 2
					//therefore adding 1 will cause at most 1 bit to be set and all others to be 0
					//If it is discontiguous, at least 2 bits will be set
					//__builtin_popcount is O(c), so this takes constant time
					//
					if(__builtin_popcount(~mask+1) > 1){
						r->contiguous = false;
					}
				}
				//Field does not have a mask
				else{

					max_length[i] = 8 * match->oxm_length;

					uint32_t value = 0;

					memcpy(&value, match->oxm_value, match->oxm_length);
					value = OS_NTOHL(value);

					r->fields[i].value = value;
					r->fields[i].mask = 0xFFFFFFFF;
					r->prefix_length[i] = 8 * match->oxm_length; // All bits specified
				}
				break;
			}
		}
		
	}
		
	//If contiguous, we need to replace value/mask with range in all fields
	if(r->contiguous){
		for(i = 0; i < NUM_FIELDS; ++i){
			// Lowest value in range is masked value
			// Highest value in range is the lowest value with all unspecified bits set
			//r->fields[i].mask = r->fields[i].value | ((~(r->fields[i].mask >> (32 - max_length[i])) << (32 - max_length[i])));
			r->fields[i].mask = r->fields[i].value | ~(r->fields[i].mask);
			//printf("%u:%u, ",r->fields[i].value, r->fields[i].mask); 
		}
		//printf("\n");
	}

	//printf("\n");

	return r;
}

//Returns true if the lagopus packet is in the desired range for each match field. 
bool rule_matches_l_packet(struct rule *rule, const struct lagopus_packet* p){
	struct flow *flow = rule->master;	
	struct match_idx_temp *idx;	
  	size_t len;

	struct match *match;
	unsigned i;	
	TAILQ_FOREACH(match, &(flow->match_list), entry){
		idx = &match_idx_temp[OXM_FIELD_TYPE(match->oxm_field)];
		len = idx->size;
		if (p->base[idx->base] == NULL) { //MATCHING ON NONEXISTANT FIELD
			return false;
		}

		point field = 0;

		memcpy(&field, p->base[idx->base] + idx->off, len);//Copy in the bytes	
		field = OS_NTOHL(field);	
		field >>= idx->shift;
		field = OS_HTONL(field);

		if (match->oxm_field & 1){
			for (i = 0; i < len; ++i) {
				if((field & 0xFF) & match->oxm_value[len + i] != match->oxm_value[i]) return false;
				field <<= 8;			
			}
		}
		else{
			if(memcmp(&field, match->oxm_value, len) != 0 ) return false;
		}
	}
	return true;
}

//Extracts the given fields from the lagopus packet and stores them in the packet, p. 
void pbuf_fill(const struct lagopus_packet *lp, uint8_t *fields, const int NUM_FIELDS, packet p){
	packet pkt = p;
	struct match_idx_temp *idx;	
	size_t len;

	for(int i = 0; i < NUM_FIELDS; ++i){
		idx = &match_idx_temp[fields[i] >> 1];
		len = idx->size;
		
		if (lp->base[idx->base] == NULL) { //EXTRACTING NONEXISTANT FIELD
			continue;
		}

		point field = 0;

		memcpy(&field, lp->base[idx->base] + idx->off, len);	
		field = OS_NTOHL(field);	
		field >>= idx->shift;

		pkt[i] = field;
	}
}
