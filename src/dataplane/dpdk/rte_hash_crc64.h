/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2013 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RTE_HASH_CRC64_H_
#define _RTE_HASH_CRC64_H_

/**
 * @file
 *
 * RTE CRC Hash using intel_hash64 algorythm.
 */

#include <rte_hash_crc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Use crc32 instruction to perform a hash.
 *
 * @param data
 *   Data to perform hash on.
 * @param data_len
 *   How many bytes to use to calculate hash value.
 * @param init_val
 *   Value to initialise hash generator.
 * @return
 *   64bit calculated hash value.
 */
static inline uint64_t
lagopus_hash_crc64(const char *data, size_t data_len, uint64_t init_val) {
  uint32_t cksum;
  unsigned i;
  uint32_t temp = 0, hash2 = 0;
  const uint32_t *p32 = (const uint32_t *)data + (data_len / 4);

  cksum = (data_len << 19) | (data_len >> 13);
  for (i = 0; i < data_len / 4; i++) {
    init_val = rte_hash_crc_4byte(*--p32, (uint32_t)init_val);
  }

  switch (3 - (data_len & 0x03)) {
    case 0:
      temp |= *((const uint8_t *)p32 + 2) << 16;
      /* Fallthrough */
    case 1:
      temp |= *((const uint8_t *)p32 + 1) << 8;
      /* Fallthrough */
    case 2:
      temp |= *((const uint8_t *)p32);
      init_val = rte_hash_crc_4byte(temp, init_val);
      hash2 = rte_hash_crc_4byte((temp << 31) | (temp >> 1), hash2);
      cksum ^= temp;
    default:
      break;
  }

  return (uint64_t)init_val << 32 | rte_hash_crc_4byte(hash2, cksum);
}

#ifdef __cplusplus
}
#endif

#endif /* _RTE_HASH_CRC64_H_ */
