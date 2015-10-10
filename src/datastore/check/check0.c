#include "lagopus_apis.h"
#include "datastore_apis.h"





#define CTOR_IDX	LAGOPUS_MODULE_CONSTRUCTOR_INDEX_BASE + 2

#define MY_CONF_NAME		"test-conf"

#define MY_COMMAND_NAME		"testobj"





static lagopus_hashmap_t s_tbl;
static datastore_interp_t s_interp;

static pthread_once_t s_once = PTHREAD_ONCE_INIT;

static void	s_ctors(void) __attr_constructor__(CTOR_IDX);
static void	s_dtors(void) __attr_destructor__(CTOR_IDX);





static lagopus_result_t
s_update(datastore_interp_t *iptr,
         datastore_interp_state_t state,
         void *obj,
         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)iptr;
  (void)state;
  (void)obj;
  (void)result;

  ret = LAGOPUS_RESULT_OK;

  return ret;
}


static lagopus_result_t
s_enable(datastore_interp_t *iptr,
         datastore_interp_state_t state,
         void *obj,
         bool *currentp,
         bool enabled,
         lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)iptr;
  (void)state;
  (void)obj;
  (void)currentp;
  (void)enabled;
  (void)result;

  ret = LAGOPUS_RESULT_OK;

  return ret;
}


static lagopus_result_t
s_serialize(datastore_interp_t *iptr,
            datastore_interp_state_t state,
            const void *obj,
            lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)iptr;
  (void)state;
  (void)obj;
  (void)result;

  ret = LAGOPUS_RESULT_OK;

  return ret;
}


static lagopus_result_t
s_destroy(datastore_interp_t *iptr,
          datastore_interp_state_t state,
          void *obj,
          lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)iptr;
  (void)state;
  (void)obj;
  (void)result;

  ret = LAGOPUS_RESULT_OK;

  return ret;
}


static lagopus_result_t
s_compare(const void *obj0, const void *obj1, int *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)obj0;
  (void)obj1;
  (void)result;

  ret = LAGOPUS_RESULT_OK;

  return ret;
}


static lagopus_result_t
s_getname(const void *obj, const char **namep) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)obj;
  (void)namep;

  ret = LAGOPUS_RESULT_OK;

  return ret;
}


static lagopus_result_t
s_duplicate(const void *obj, const char *namespace) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;

  (void)obj;
  (void)namespace;

  ret = LAGOPUS_RESULT_OK;

  return ret;
}


static lagopus_result_t
s_parse(datastore_interp_t *iptr,
        datastore_interp_state_t state,
        size_t argc, const char *const argv[],
        lagopus_hashmap_t *hptr,
        datastore_update_proc_t u_proc,
        datastore_enable_proc_t e_proc,
        datastore_serialize_proc_t s_proc,
        datastore_destroy_proc_t d_proc,
        lagopus_dstring_t *result) {
  lagopus_result_t ret = LAGOPUS_RESULT_ANY_FAILURES;
  size_t i;

  (void)iptr;
  (void)state;
  (void)hptr;
  (void)u_proc;
  (void)e_proc;
  (void)s_proc;
  (void)d_proc;
  (void)result;

  for (i = 0; i < argc; i++) {
    fprintf(stderr, "argv[" PFSZS(4, u) "]:\t'%s'\n", i, argv[i]);
  }

  ret = LAGOPUS_RESULT_OK;

  return ret;
}





static void
s_child_at_fork(void) {
  lagopus_hashmap_atfork_child(&s_tbl);
}


static void
s_once_proc(void) {
  lagopus_result_t r;

  if ((r = lagopus_hashmap_create(&s_tbl, LAGOPUS_HASHMAP_TYPE_STRING,
                                  NULL)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't initialize an object table.\n");
  }

  if ((r = datastore_register_table(MY_COMMAND_NAME,
                                    &s_tbl,
                                    s_update,
                                    s_enable,
                                    s_serialize,
                                    s_destroy,
                                    s_compare,
                                    s_getname,
                                    s_duplicate)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register an object table for \"%s\".\n",
                       MY_COMMAND_NAME);
  }

  if ((r = datastore_create_interp(&s_interp)) != LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't create the datastore interpretor.\n");
  }

  if ((r = datastore_interp_register_command(&s_interp, NULL,
           MY_COMMAND_NAME, s_parse)) !=
      LAGOPUS_RESULT_OK) {
    lagopus_perror(r);
    lagopus_exit_fatal("can't register an object command \"%s\".\n",
                       MY_COMMAND_NAME);
  }

  (void)pthread_atfork(NULL, NULL, s_child_at_fork);
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  lagopus_msg_debug(10, "datastore object \"%s\" initialzied.\n",
                    MY_COMMAND_NAME);
}


static inline void
s_final(void) {
  datastore_destroy_interp(&s_interp);
  lagopus_hashmap_destroy(&s_tbl, true);
}


static void
s_dtors(void) {
  s_final();

  lagopus_msg_debug(10, "datastore object \"%s\" finalized.\n",
                    MY_COMMAND_NAME);
}





int
main(int argc, const char *const argv[]) {
  lagopus_result_t r = LAGOPUS_RESULT_ANY_FAILURES;
  const char *file = argv[1];

  (void)argc;

  if ((r = datastore_register_configurator(MY_CONF_NAME)) ==
      LAGOPUS_RESULT_OK) {

    lagopus_dstring_t ds;
    char *str = NULL;

    (void)lagopus_dstring_create(&ds);

    if (IS_VALID_STRING(file) == true) {
      r = datastore_interp_eval_file(&s_interp, MY_CONF_NAME, file, &ds);
    } else {
      while ((r = datastore_interp_eval_fd(&s_interp, MY_CONF_NAME,
                                           stdin, stdout, false, &ds)) ==
             LAGOPUS_RESULT_OK) {
        lagopus_dstring_clear(&ds);
      }
    }

    if (r != LAGOPUS_RESULT_OK) {
      lagopus_perror(r);
    }

    (void)lagopus_dstring_str_get(&ds, &str);
    if (IS_VALID_STRING(str) == true) {
      fprintf(stdout, "%s", str);
    }

    free((void *)str);

  } else {
    lagopus_perror(r);
  }

  datastore_unregister_configurator(MY_CONF_NAME);

  return (r == LAGOPUS_RESULT_OK) ? 0 : 1;
}
