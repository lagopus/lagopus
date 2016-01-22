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



#define RESULT_NULL	"\"\""





static lagopus_result_t
s_parse_lock(datastore_interp_t *iptr,
             datastore_interp_state_t state,
             size_t argc, const char *const argv[],
             lagopus_hashmap_t *hptr,
             datastore_update_proc_t proc,
             lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  (void)iptr;
  (void)state;
  (void)hptr;
  (void)proc;

  for (i = 0; i < argc; i++) {
    lagopus_msg_debug(1, "argv[" PFSZS(4, u) "]:\t'%s'\n", i, argv[i]);
  }

  argv++;
  if (IS_VALID_STRING(*argv) == false) {
    /*
     * Returns the current lock parameters.
     */


  } else {

    while (IS_VALID_STRING(*argv) == true) {

    }

  }
  /*
   * Failsafe.
   */
  ret = datastore_json_result_set(result, ret, RESULT_NULL);

  return ret;
}





#undef RESULT_NULL




