#include "lagopus_apis.h"
#include "lagopus/ofp_bridgeq_mgr.h"
#include "datastore_internal.h"
#include "datastore_apis.h"
#include "../../agent/channel_mgr.h"
#include "lagopus/dp_apis.h"

#define CTOR_IDX	LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 2

#define MY_CONF_NAME		"test-conf"

extern datastore_interp_t datastore_get_master_interp(void);

static datastore_interp_t interp;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static void	ctors(void) __attr_constructor__(CTOR_IDX);
static void	dtors(void) __attr_destructor__(CTOR_IDX);

static void
once_proc(void) {
  lagopus_result_t ret;

  interp = datastore_get_master_interp();
  if (interp == NULL) {
    lagopus_exit_fatal("can't get the datastore interpretor.\n");
  }

  if ((ret = dp_api_init()) != LAGOPUS_RESULT_OK) {
    lagopus_perror(ret);
    lagopus_exit_fatal("can't init dp_api.\n");
  }

  ofp_bridgeq_mgr_initialize(NULL);
  channel_mgr_initialize();
}

static inline void
init(void) {
  (void) pthread_once(&once, once_proc);
}

static void
ctors(void) {
  init();

  lagopus_msg_debug(10, "initialzied.\n");
}

static inline void
final(void) {
  ofp_bridgeq_mgr_destroy();
  dp_api_fini();
}

static void
dtors(void) {
  final();

  lagopus_msg_debug(10, "finalized.\n");
}



static void
check_serialize(void) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  lagopus_dstring_t ds = NULL;
  char *str = NULL;

  if ((r = lagopus_dstring_create(&ds)) == LAGOPUS_RESULT_OK) {
    (void)lagopus_dstring_clear(&ds);
    if ((r = datastore_interp_eval_string(&interp, MY_CONF_NAME,
                                          "save /dev/stdout", &ds)) !=
        LAGOPUS_RESULT_OK) {
      lagopus_perror(r);
      (void) lagopus_dstring_str_get(&ds, &str);
      if (IS_VALID_STRING(str) == true) {
        fprintf(stdout, "%s", str);
      }
      free(str);
    }
  } else {
    lagopus_perror(r);
  }
  if (ds != NULL) {
    lagopus_dstring_destroy(&ds);
  }
}

int
main(int argc, const char *const argv[]) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  const char *file = argv[1];
  (void) argc;

  if ((ret = datastore_all_commands_initialize()) ==
      LAGOPUS_RESULT_OK) {
    if ((ret = datastore_register_configurator(MY_CONF_NAME)) ==
        LAGOPUS_RESULT_OK) {

      lagopus_dstring_t ds = NULL;
      char *str = NULL;

      (void) lagopus_dstring_create(&ds);

      if (IS_VALID_STRING(file) == true) {
        ret = datastore_interp_eval_file(&interp, MY_CONF_NAME, file, &ds);
      } else {
        while ((ret = datastore_interp_eval_fd(&interp, MY_CONF_NAME,
                                               stdin, stdout, false, &ds)) ==
               LAGOPUS_RESULT_OK ||
               ret == LAGOPUS_RESULT_DATASTORE_INTERP_ERROR ||
               ret == LAGOPUS_RESULT_NOT_FOUND) {
          lagopus_dstring_clear(&ds);
        }
      }

      if (ret != LAGOPUS_RESULT_OK) {
        lagopus_perror(ret);
      }

      (void) lagopus_dstring_str_get(&ds, &str);
      if (IS_VALID_STRING(str) == true) {
        fprintf(stdout, "%s", str);
      }

      if (str != NULL) {
        free(str);
      }
      if (ds != NULL) {
        lagopus_dstring_destroy(&ds);
      }
    } else {
      lagopus_perror(ret);
    }

    check_serialize();

    datastore_unregister_configurator(MY_CONF_NAME);
  } else {
    lagopus_perror(ret);
  }

  return (ret == LAGOPUS_RESULT_OK) ? 0 : 1;
}
