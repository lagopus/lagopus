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


#include <stdio.h>
#include <grp.h>
#include <unistd.h>
#include <sys/stat.h>
#include "chgroup.h"

/* Change group and set file permission. */
void
chgroup(const char *path, const char *group_str) {
  struct group *group;

  /* Change group. */
  group = getgrnam(group_str);
  if (group != NULL) {
    chown(path, (uid_t)-1, group->gr_gid);
  }

  /* Change file mod to group readable/writable. */
  chmod(path, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
}
