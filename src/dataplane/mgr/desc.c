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

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/utsname.h>

#include "lagopus_apis.h"
#include "openflow.h"
#include "lagopus/ofp_dp_apis.h"

#include "lagopus/dataplane.h"

#define CPUINFO_KEY "model name\t: "

const char *dmidecode_param[] = {
  "system-manufacturer",
  "system-product-name",
  "processor-version"
};

static const char *
get_dmi_string(const char *param) {
  static char buf[128];
  char cmd[128];
  FILE *fp;

  snprintf(cmd, sizeof(cmd), "/usr/sbin/dmidecode -s %s", param);
  fp = popen(cmd, "r");
  if (fp != NULL) {
    fgets(buf, sizeof(buf), fp);
    fclose(fp);
  } else {
    strncpy(buf, "unknown ", sizeof(buf));
  }
  return buf;
}

static void
copy_hw_info(char *buf, size_t len) {
  unsigned i;

  buf[0] = '\0';
  for (i = 0; i < sizeof(dmidecode_param) / sizeof(dmidecode_param[0]); i++) {
    strncat(buf, get_dmi_string(dmidecode_param[i]), len - 1);
    buf[strlen(buf) - 1] = ' ';
    buf[strlen(buf)] = '\0';
  }
  buf[strlen(buf) - 1] = '\0';
}

static void
copy_os_info(char *buf, size_t len) {
  struct utsname ubuf;

  if (uname(&ubuf) < 0) {
    strncpy(buf, "unknown", len);
    return;
  }
  snprintf(buf, len, "%s %s %s %s %s",
           ubuf.sysname,
           ubuf.nodename,
           ubuf.release,
           ubuf.version,
           ubuf.machine);
}

/*
 * description
 */
lagopus_result_t
ofp_desc_get(uint64_t dpid,
             struct ofp_desc *desc,
             struct ofp_error *error) {
  (void) dpid;

  strncpy(desc->mfr_desc,
          "Lagopus project",
          DESC_STR_LEN);

  copy_hw_info(desc->hw_desc, DESC_STR_LEN);

  copy_os_info(desc->sw_desc, DESC_STR_LEN);

  strncpy(desc->serial_num,
          "0",
          SERIAL_NUM_LEN);

  copy_dataplane_info(desc->dp_desc, DESC_STR_LEN);

  (void) error;
  return LAGOPUS_RESULT_OK;
}
