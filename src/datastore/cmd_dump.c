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

#include "lagopus_apis.h"
#include "datastore_apis.h"
#include "cmd_common.h"
#include "cmd_dump.h"

lagopus_result_t
cmd_dump_file_write(FILE *fp,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;

  if (fp != NULL && result != NULL) {
    ret = lagopus_dstring_str_get(result, &str);
    if (ret == LAGOPUS_RESULT_OK) {
      if (fputs(str, fp) != EOF) {
        ret = LAGOPUS_RESULT_OK;
      } else {
        ret = LAGOPUS_RESULT_OUTPUT_FAILURE;
        lagopus_perror(ret);
      }
    } else {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  free(str);

  return ret;
}

lagopus_result_t
cmd_dump_file_send(datastore_interp_t *iptr,
                   FILE *fp,
                   void *stream_out,
                   datastore_printf_proc_t printf_proc,
                   lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char buf[65536];

  if (iptr != NULL && fp != NULL && stream_out != NULL &&
      printf_proc != NULL && result != NULL) {
    if (printf_proc(stream_out,
                    "{\"ret\":\"OK\",\n"
                    "\"data\":") < 0) {
      lagopus_msg_warning("Can't send data.\n");
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
      goto done;
    }

    rewind(fp);
    while (fgets(buf, sizeof(buf), fp) != NULL) {
      lagopus_msg_debug(10, "send data : %s\n", buf);
      if (printf_proc(stream_out, "%s", buf) < 0) {
        lagopus_msg_warning("Can't send data.\n");
        ret = LAGOPUS_RESULT_INVALID_OBJECT;
        goto done;
      }
    }

    if (printf_proc(stream_out, "}\n") < 0) {
      lagopus_msg_warning("Can't send data.\n");
      ret = LAGOPUS_RESULT_INVALID_OBJECT;
      goto done;
    }

    ret = LAGOPUS_RESULT_OK;
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

done:
  return ret;
}

lagopus_result_t
cmd_dump_error_send(void *stream_out,
                    datastore_printf_proc_t printf_proc,
                    lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  char *str = NULL;

  if (stream_out != NULL && printf_proc != NULL &&
      result != NULL) {
    ret = lagopus_dstring_str_get(result, &str);
    if (ret == LAGOPUS_RESULT_OK) {
      printf_proc(stream_out,"%s\n", str);
    } else {
      lagopus_perror(ret);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  free(str);

  return ret;
}

lagopus_result_t
cmd_dump_main(lagopus_thread_t *thd,
              datastore_interp_t *iptr,
              void *conf,
              void *stream_out,
              datastore_printf_proc_t printf_proc,
              datastore_config_type_t ftype,
              char *file_name,
              bool is_with_stats,
              cmd_dump_proc_t dump_proc) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t result = NULL;
  FILE *fp = NULL;
  mode_t old_mask;
  int fd;

  if (thd != NULL && iptr != NULL && conf != NULL &&
      stream_out !=NULL && printf_proc != NULL &&
      file_name != NULL) {
    /* start : sending large data. */
    if (ftype == DATASTORE_CONFIG_TYPE_STREAM_SESSION) {
      if ((ret = datastore_interp_blocking_session_set(
              iptr, (struct session *) stream_out, thd)) !=
          LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
        goto done;
      }
    }

    if ((ret = lagopus_dstring_create(&result)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(ret);
      goto done;
    }

    old_mask = umask(S_IRUSR | S_IWUSR);
    fd = mkstemp(file_name);
    (void) umask(old_mask);
    if (fd >= 0) {
      if ((fp = fdopen(fd, "w+")) != NULL) {
        unlink(file_name);
        ret = dump_proc(iptr, conf, fp,
                        stream_out, printf_proc,
                        is_with_stats,
                        &result);
        if (ret != LAGOPUS_RESULT_OK) {
          lagopus_perror(ret);
        }
        fclose(fp);
      } else {
        close(fd);
        ret = LAGOPUS_RESULT_POSIX_API_ERROR;
        lagopus_perror(ret);
      }
    } else {
      ret = LAGOPUS_RESULT_POSIX_API_ERROR;
      lagopus_perror(ret);
    }

 done:
    lagopus_dstring_destroy(&result);
    /* end : sending large data. */
    if (ftype == DATASTORE_CONFIG_TYPE_STREAM_SESSION) {
      datastore_interp_blocking_session_unset(
          iptr,(struct session *) stream_out);
    }
  } else {
    ret = LAGOPUS_RESULT_INVALID_ARGS;
    lagopus_perror(ret);
  }

  return ret;
}
