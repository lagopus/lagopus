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

#ifndef __DATASTORE_INTERP_STATE_H__
#define __DATASTORE_INTERP_STATE_H__





typedef enum {
  DATASTORE_INTERP_STATE_UNKNOWN = 0,
  DATASTORE_INTERP_STATE_PRELOAD,
  DATASTORE_INTERP_STATE_AUTO_COMMIT,
  DATASTORE_INTERP_STATE_DRYRUN,
  DATASTORE_INTERP_STATE_ATOMIC,
  DATASTORE_INTERP_STATE_COMMITING,
  DATASTORE_INTERP_STATE_COMMITED,
  DATASTORE_INTERP_STATE_COMMIT_FAILURE,
  DATASTORE_INTERP_STATE_ABORTING,
  DATASTORE_INTERP_STATE_ABORTED,
  DATASTORE_INTERP_STATE_ROLLBACKING,
  DATASTORE_INTERP_STATE_ROLLBACKED,
  DATASTORE_INTERP_STATE_SHUTDOWN,
  DATASTORE_INTERP_STATE_DESTROYING,
} datastore_interp_state_t;





#endif /* ! __DATASTORE_INTERP_STATE_H__ */
