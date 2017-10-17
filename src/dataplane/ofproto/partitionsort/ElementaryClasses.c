#include "ElementaryClasses.h"
#include "../City.h"
#include <stdio.h>

struct match_idx_temp;

//Taken from thtable.c
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

//Taken from thtable.c
static struct match *
ps_copy_masked_match(const struct match *src_match) {
  struct match *dst_match;
  struct match_idx_temp *idx;
  size_t len;
  uint64_t val64;
  uint32_t val32;
  uint16_t val16;

  len = OXM_MATCH_VALUE_LEN(src_match);
  dst_match = calloc(1, sizeof(struct match) + len * 2);
  if (dst_match == NULL) {
    return NULL;
  }
  memcpy(dst_match, src_match, sizeof(struct match));
  idx = &match_idx_temp[OXM_FIELD_TYPE(src_match->oxm_field)];
  if (idx->size == len && idx->shift == 0) {
    memcpy(dst_match->oxm_value, src_match->oxm_value, len);
  } else {
    switch (len) {
      case 8:
        memcpy(&val64, src_match->oxm_value, sizeof(val64));
        val64 = OS_NTOHLL(val64);
        val64 <<= idx->shift;
        val64 = OS_HTONLL(val64);
        memcpy(dst_match->oxm_value, &val64, sizeof(val64));
        break;
      case 4:
        memcpy(&val32, src_match->oxm_value, sizeof(val32));
        val32 = OS_NTOHL(val32);
        val32 <<= idx->shift;
        val32 = OS_HTONL(val32);
        memcpy(dst_match->oxm_value, &val32, sizeof(val32));
        break;
      case 2:
        memcpy(&val16, src_match->oxm_value, sizeof(val16));
        val16 = OS_NTOHS(val16);
        val16 <<= idx->shift;
        val16 = OS_HTONS(val16);
        memcpy(dst_match->oxm_value, &val16, sizeof(val16));
        break;
      case 1:
        *dst_match->oxm_value = (*src_match->oxm_value) << idx->shift;
        break;
    }
  }
  if (!OXM_FIELD_HAS_MASK(src_match->oxm_field)) {
    memcpy(&dst_match->oxm_value[idx->size], idx->mask, idx->size);
  } else if (idx->shift == 0) {
    memcpy(&dst_match->oxm_value[idx->size],
           &src_match->oxm_value[len + (len - idx->size)],
           idx->size);
  } else {
    switch (len) {
      case 8:
        memcpy(&val64, &src_match->oxm_value[len], sizeof(val64));
        val64 = OS_NTOHLL(val64);
        val64 <<= idx->shift;
        val64 = OS_HTONLL(val64);
        memcpy(&dst_match->oxm_value[idx->size], &val64, sizeof(val64));
        break;
      case 4:
        memcpy(&val32, &src_match->oxm_value[len], sizeof(val32));
        val32 = OS_NTOHL(val32);
        val32 <<= idx->shift;
        val32 = OS_HTONL(val32);
        memcpy(&dst_match->oxm_value[idx->size], &val32, sizeof(val32));
        break;
      case 2:
        memcpy(&val16, &src_match->oxm_value[len], sizeof(val16));
        val16 = OS_NTOHS(val16);
        val16 <<= idx->shift;
        val16 = OS_HTONS(val16);
        memcpy(&dst_match->oxm_value[idx->size], &val16, sizeof(val16));
        break;
      case 1:
        dst_match->oxm_value[idx->size] =
            src_match->oxm_value[len] << idx->shift;
        break;
    }
  }
  dst_match->oxm_length = idx->size;
  return dst_match;
}

long long ps_hash_flow(struct flow *flow){
	struct match *match = NULL;
	long long key = -1;
	TAILQ_FOREACH(match, &flow->match_list, entry){
		
		struct match *pmatch;

		pmatch = ps_copy_masked_match(match);
		key = CityHash64WithSeed(pmatch->oxm_value, pmatch->oxm_length, key);//Calculates hash using hash key from last match as seed
		free(pmatch);	
	}
	return key;
}

rule* rule_create(int dimension){ 
	rule *r = (rule *)safe_malloc(sizeof(rule));	
	r->dim = dimension;

	//Allocates a range array with all values set to 0. THIS MAY BE INEFFICIENT
	r->range = (point **)malloc(dimension * sizeof(point *));
	for(int  i = 0; i < dimension; ++i){
		r->range[i] = (point *)calloc(2, sizeof(point));
	}
	
	//Allocates a prefix_length array with all values set to 0
	r->prefix_length = (unsigned *)calloc(dimension, sizeof(unsigned));
	
	r->marked_delete = 0;
	r->priority = 0; 
	r->master = NULL;
	return r;
}

//Converts a flow to a rule considering only the fields specified 
rule* rule_init_c(const struct flow *flow, uint8_t *fields, const int NUM_FIELDS){
	rule *r = (rule*)safe_malloc(sizeof(rule));	
	r->dim = NUM_FIELDS;

	struct match *match = NULL;
	int count = 0, i = 0, j = 0;
	uint64_t key = 0;

	
	r->range = (point **)malloc(NUM_FIELDS * sizeof(point *));//Allocate pointer memory for each range
	for(i = 0; i < NUM_FIELDS; ++i){
		r->range[i] = (point *)calloc(2, sizeof(point));//Allocate the memory for this range (DEFAULTS TO [0,0])
	}

	r->prefix_length = (unsigned *)malloc(NUM_FIELDS * sizeof(unsigned));//Allocate memory for each prefix length

	r->priority = flow->priority;
	r->master = flow;//Must keep track of this to match against a packet

	if(fields == NULL)
		return r; //No reason to extract fields, none to extract

	//Loop through all matching fields and extract the desired fields
	TAILQ_FOREACH(match, &flow->match_list, entry){			

		if(count == NUM_FIELDS)
			break;
								
		for(i = 0; i < NUM_FIELDS; ++i){

			//If the field is a match field
			if(match->oxm_field == fields[i]){

				++count;
				r->prefix_length[i] = match->oxm_length;

				//If the field has a bitmask, convert to range
				if(match->oxm_field & 1){
					uint32_t value = 0;
					uint32_t mask = 0;

					memcpy(&value, match->oxm_value, match->oxm_length/2);
					memcpy(&mask, &match->oxm_value[match->oxm_length/2], match->oxm_length/2);

					value = OS_NTOHL(value);
					mask = OS_NTOHL(mask);

					//Lowest masked value is lowest in range, adding complement of mask gives the highest value
					r->range[i][low_dim] = value & mask;
					r->range[i][high_dim] = (value & mask)|~(mask);
				}
				//Field does not have a range
				else{
					uint32_t value = 0;

					memcpy(&value, match->oxm_value, match->oxm_length);
					value = OS_NTOHL(value);

					r->range[i][low_dim] = value;
					r->range[i][high_dim] = value;
				}
				break;
			}
		}
		
		//struct match *pmatch;

		//pmatch = ps_copy_masked_match(match);
		//key = CityHash64WithSeed(pmatch->oxm_value, pmatch->oxm_length, key);//Calculates hash using hash key from last match as seed
		//free(pmatch);	
	}

	//r->priority = key;//Hash key is unique for each flow, priority is not
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
void pbuf_init(const struct lagopus_packet *lp, uint8_t *fields, const int NUM_FIELDS, packet p){
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
