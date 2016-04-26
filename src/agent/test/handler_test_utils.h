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

#ifndef __OFP_HANDLER_TEST_UTIL_H__
#define __OFP_HANDLER_TEST_UTIL_H__

#include "lagopus_apis.h"
#include "../channel.h"
#include "lagopus/ofp_dp_apis.h"

#define CHANNEL_MAX_NUM 5

/* define procs */
typedef lagopus_result_t \
(*ofp_handler_proc_t)(struct channel *channel, struct pbuf *pbuf,\
                      struct ofp_header *xid_header, struct ofp_error *error);

typedef lagopus_result_t \
(*ofp_reply_create_proc_t)(struct channel *channel, struct pbuf **pbuf, \
                           struct ofp_header *xid_header);
typedef lagopus_result_t \
(*ofp_reply_create_expect_error_proc_t)(struct channel *channel,        \
                                        struct pbuf **pbuf,             \
                                        struct ofp_header *xid_header,  \
                                        struct ofp_error *error);
typedef lagopus_result_t \
(*ofp_send_proc_t)(struct channel *channel, struct ofp_header *xid_header);
typedef lagopus_result_t \
(*ofp_channels_proc_t)(struct channel **channels,
                       struct ofp_header *xid_headers);
typedef lagopus_result_t \
(*ofp_channels_send_proc_t)(struct channel **channels, \
                            struct ofp_header *xid_headers, \
                            struct pbuf *pbuf);
typedef lagopus_result_t \
(*ofp_check_packet_proc_t)(struct channel *channel, struct pbuf **pbuf, \
                           struct ofp_header *xid_header);
typedef lagopus_result_t \
(*ofp_channels_write_proc_t)(struct channel **channels, \
                             struct ofp_header *xid_headers);
typedef lagopus_result_t \
(*ofp_reply_list_create_proc_t)(struct channel *channel, \
                                struct pbuf_list **pbuf_list, \
                                struct ofp_header *xid_header);
typedef void \
(*ofp_data_create_proc_t)(struct channel **channels, \
                          struct ofp_header *xid_headers, \
                          struct pbuf *pbuf);

typedef lagopus_result_t \
(*ofp_instruction_parse_proc_t)(struct pbuf *pbuf, \
                                struct instruction_list *instruction_list, \
                                struct ofp_error *error);

typedef lagopus_result_t \
(*ofp_match_parse_proc_t)(struct channel *channel, \
                          struct pbuf *pbuf, \
                          struct match_list *match_list, \
                          struct ofp_error *error);

/* check functions */
lagopus_result_t
check_packet_parse(ofp_handler_proc_t handler_proc,
                   const char packet[]);

lagopus_result_t
check_packet_parse_expect_error(ofp_handler_proc_t handler_proc,
                                const char packet[],
                                const struct ofp_error *error);

lagopus_result_t
check_packet_parse_cont(ofp_handler_proc_t handler_proc,
                        const char packet[]);

lagopus_result_t
check_packet_parse_expect_error_cont(ofp_handler_proc_t handler_proc,
                                     const char packet[],
                                     const struct ofp_error *error);

lagopus_result_t
check_packet_parse_array(ofp_handler_proc_t handler_proc,
                         const char *packet[],
                         int array_len);

lagopus_result_t
check_packet_parse_array_expect_error(ofp_handler_proc_t handler_proc,
                                      const char *packet[],
                                      int array_len,
                                      const struct ofp_error *error);

lagopus_result_t
check_packet_parse_array_for_multipart(ofp_handler_proc_t handler_proc,
                                       const char *packet[],
                                       int array_len);

lagopus_result_t
check_packet_parse_array_for_multipart_expect_error(ofp_handler_proc_t
    handler_proc,
    const char *packet[],
    int array_len,
    const struct ofp_error *error);

lagopus_result_t
check_packet_parse_with_dequeue(ofp_handler_proc_t handler_proc,
                                const char packet[],
                                void **get);

lagopus_result_t
check_packet_parse_with_dequeue_expect_error(ofp_handler_proc_t handler_proc,
    const char packet[],
    void **get,
    const struct ofp_error *error);


lagopus_result_t
check_packet_create(ofp_reply_create_proc_t create_proc,
                    const char packet[]);

lagopus_result_t
check_packet_create_expect_error(ofp_reply_create_expect_error_proc_t
                                 create_proc,
                                 const char packet[],
                                 struct ofp_error *expected_error);

lagopus_result_t
check_use_channels(ofp_channels_proc_t channels_proc);

lagopus_result_t
check_use_channels_send(ofp_channels_send_proc_t send_proc,
                        const char packet[]);

lagopus_result_t
check_packet(ofp_check_packet_proc_t check_packet_proc,
             const char packet[]);

lagopus_result_t
check_use_channels_write(ofp_channels_write_proc_t write_proc,
                         const char packet[]);
lagopus_result_t
check_packet_send(ofp_send_proc_t send_proc);

lagopus_result_t
check_pbuf_list_packet_create(ofp_reply_list_create_proc_t create_proc,
                              const char *packet_array[],
                              int array_len);

lagopus_result_t
check_pbuf_list_across_packet_create(ofp_reply_list_create_proc_t create_proc,
                                     const char *header_data[],
                                     const char *body_data[],
                                     size_t nums[],
                                     int array_len);
void
data_create(ofp_data_create_proc_t create_proc,
            const char packet[]);

struct channel *
create_data_channel(void);

void
destroy_data_channel(struct channel *channel);

void
create_packet(const char in_data[], struct pbuf **pbuf);

lagopus_result_t
check_instruction_parse_expect_error(ofp_instruction_parse_proc_t
                                     instruction_parse_proc,
                                     struct pbuf *test_pbuf,
                                     struct instruction_list *instruction_list,
                                     struct ofp_error *expected_error);

lagopus_result_t
check_match_parse_expect_error(ofp_match_parse_proc_t match_parse_proc,
                               struct channel *channel,
                               struct pbuf *test_pbuf,
                               struct match_list *match_list,
                               struct ofp_error *expected_error);

lagopus_result_t
ofp_header_decode_sneak_test(struct pbuf *pbuf,
                             struct ofp_header *packet);

#endif /* __OFP_HANDLER_TEST_UTIL_H__ */
