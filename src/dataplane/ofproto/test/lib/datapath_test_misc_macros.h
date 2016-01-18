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

/**
 * @file	datapath_test_misc_macros.h
 */

#ifndef __DATAPLANE_TEST_MISC_MACROS_H__
#define __DATAPLANE_TEST_MISC_MACROS_H__

/*
 * The miscellaneous macros found duplicated across the datapath/dpmgr
 * test sources.  Such the macros should be moved here so that we
 * avoid code duplication.
 *
 * The declarations and definitions should be sorted
 * lexicographically.
 *
 * Functions should be placed in datapath_test_misc.h so that we
 * prevent namespace pollution.
 */


/*
 * Constants.
 */

/*
 * Assertion message buffer size.
 */
#define TEST_ASSERT_MESSAGE_BUFSIZE	(1024)


/*
 * Tools.
 */

/* Find the array length. */
#define ARRAY_LEN(_a) (sizeof(_a) / sizeof((_a)[0]))

/* Dump a flowdb. */
#define FLOWDB_DUMP(_flowdb, _msg, _fp)			\
  do {							\
    fprintf((_fp), "%s: %s:\n", __func__, (_msg));	\
    flowdb_dump((_flowdb), (_fp));			\
  } while (0)

/*
 * Return the _idx-th byte of an integer _d in the network byte
 * order.
 */
#define NBO_BYTE(_d, _idx) (((_d) >> ((sizeof(_d) - (_idx) - 1) * 8)) & 0xff)

/* Count a tail queue. */
#define TAILQ_COUNT(_count, _type, _head, _entry)	\
  do {							\
    _type *_e;						\
    (_count) = 0;					\
    TAILQ_FOREACH(_e, (_head), _entry)			\
    (_count)++;					\
  } while (0)

/*
 * Get the *pointer* to the table pointer in a flow.
 * No side effects unlike table_get().
 */
#define	TABLE_GET(_tp, _fdb, _id)	\
  do {					\
    (_tp) = &(_fdb)->tables[(_id)];	\
  } while (0)

/* Find an instruction of the given type. */
#define _IL_FIND_INSN(_insn, _type, _il, _entry)\
  do {								\
    TAILQ_FOREACH(_insn, (_il), _entry) {			\
      if ((_type) == _insn->ofpit.type)				\
        break;							\
    }								\
  } while (0)


/*
 * Unity assertions.
 */

/* Assert a working flow add. */
#define TEST_ASSERT_FLOW_ADD_OK(_bg, _fm, _ml, _il, _er)	\
  do {								\
    lagopus_result_t _rv;					\
    _rv = flowdb_flow_add((_bg), (_fm), (_ml), (_il), (_er));	\
    TEST_ASSERT_EQUAL(_rv, LAGOPUS_RESULT_OK);			\
  } while (0)

/* Assert a failing flow add. */
#define TEST_ASSERT_FLOW_ADD_NG(_bg, _fm, _ml, _il, _er)	\
  do {								\
    lagopus_result_t _rv;					\
    _rv = flowdb_flow_add((_bg), (_fm), (_ml), (_il), (_er));	\
    TEST_ASSERT_NOT_EQUAL(_rv, LAGOPUS_RESULT_OK);		\
  } while (0)

/* Assert a working flow modify. */
#define TEST_ASSERT_FLOW_MODIFY_OK(_bg, _fm, _ml, _il, _er)		\
  do {									\
    lagopus_result_t _rv;						\
    _rv = flowdb_flow_modify((_bg), (_fm), (_ml), (_il), (_er));	\
    TEST_ASSERT_EQUAL(_rv, LAGOPUS_RESULT_OK);				\
  } while (0)

/* Assert a failing flow modify. */
#define TEST_ASSERT_FLOW_MODIFY_NG(_bg, _fm, _ml, _il, _er)		\
  do {									\
    lagopus_result_t _rv;						\
    _rv = flowdb_flow_modify((_bg), (_fm), (_ml), (_il), (_er));	\
    TEST_ASSERT_NOT_EQUAL(_rv, LAGOPUS_RESULT_OK);			\
  } while (0)

/* Assert a working flow delete. */
#define TEST_ASSERT_FLOW_DELETE_OK(_bg, _fm, _ml, _er)		\
  do {								\
    lagopus_result_t _rv;					\
    _rv = flowdb_flow_delete((_bg), (_fm), (_ml), (_er));	\
    TEST_ASSERT_EQUAL(_rv, LAGOPUS_RESULT_OK);			\
  } while (0)

/* Assert a failing flow delete. */
#define TEST_ASSERT_FLOW_DELETE_NG(_bg, _fm, _ml, _er)		\
  do {								\
    lagopus_result_t _rv;					\
    _rv = flowdb_flow_delete((_bg), (_fm), (_ml), (_er));	\
    TEST_ASSERT_NOT_EQUAL(_rv, LAGOPUS_RESULT_OK);		\
  } while (0)

/* Assert a working flow stats. */
#define TEST_ASSERT_FLOW_STATS_OK(_fd, _rq, _ml, _fsl, _er)		\
  do {									\
    lagopus_result_t _rv;						\
    _rv = flowdb_flow_stats((_fd), (_rq), (_ml), (_fsl), (_er));	\
    TEST_ASSERT_EQUAL(_rv, LAGOPUS_RESULT_OK);				\
  } while (0)

/* Assert a failing flow stats. */
#define TEST_ASSERT_FLOW_STATS_NG(_fd, _rq, _ml, _fsl, _er)		\
  do {									\
    lagopus_result_t _rv;						\
    _rv = flowdb_flow_stats((_fd), (_rq), (_ml), (_fsl), (_er));	\
    TEST_ASSERT_NOT_EQUAL(_rv, LAGOPUS_RESULT_OK);			\
  } while (0)

/* Assert a flow counter. */
#define TEST_ASSERT_FLOWINFO_NFLOW(_fi, _num, _msg)		\
  do {								\
    char ___buf[TEST_ASSERT_MESSAGE_BUFSIZE];			\
    \
    snprintf(___buf, sizeof(___buf), "%s, flow count", (_msg)); \
    TEST_ASSERT_EQUAL_INT_MESSAGE(_num, (_fi)->nflow, ___buf);	\
  } while (0)

/* Positively assert a flow addition to a flowinfo. */
#define TEST_ASSERT_FLOWINFO_ADD_OK(_fi, _fl, _msg)                     \
    do {                                                                \
    char ___buf[TEST_ASSERT_MESSAGE_BUFSIZE];                           \
    lagopus_result_t _rv;                                               \
                                                                        \
    _rv = (_fi)->add_func(_fi, (_fl));                                  \
    snprintf(___buf, sizeof(___buf), "%s, %d: positive flow addition", (_msg), (_rv)); \
    TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == _rv, ___buf);         \
    } while (0)

/* Negatively assert a flow addition to a flowinfo. */
/* XXX seems this never happens. */
#define TEST_ASSERT_FLOWINFO_ADD_NG(_fi, _fl, _msg)			\
  do {									\
    char ___buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(___buf, sizeof(___buf), "%s, negative flow addition", (_msg)); \
    TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != (_fi)->add_func(_fi, (_fl)), ___buf);	\
  } while (0)

/* Positively assert a flow deletion to a flowinfo. */
#define TEST_ASSERT_FLOWINFO_DEL_OK(_fi, _fl, _msg)			\
  do {									\
    char ___buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(___buf, sizeof(___buf), "%s, positive flow deletion", (_msg)); \
    TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK == (_fi)->del_func(_fi, (_fl)), ___buf); \
  } while (0)

/* Negatively assert a flow deletion to a flowinfo. */
#define TEST_ASSERT_FLOWINFO_DEL_NG(_fi, _fl, _msg)			\
  do {									\
    char ___buf[TEST_ASSERT_MESSAGE_BUFSIZE];				\
    \
    snprintf(___buf, sizeof(___buf), "%s, negative flow deletion", (_msg)); \
    TEST_ASSERT_TRUE_MESSAGE(LAGOPUS_RESULT_OK != (_fi)->del_func(_fi, (_fl)), ___buf); \
  } while (0)

/* Positively assert a flow existence in a flowinfo. */
#define TEST_ASSERT_FLOWINFO_FIND_OK(_fi, _fl, _msg)	\
  do {							\
    const struct flow *__fl;				\
    __fl = (_fi)->find_func((_fi), (_fl));		\
    TEST_ASSERT_NOT_NULL_MESSAGE(__fl, (_msg));		\
    TEST_ASSERT_TRUE_MESSAGE(__fl == (_fl), (_msg));	\
  } while (0)

/* Negatively assert a flow existence in a flowinfo. */
#define TEST_ASSERT_FLOWINFO_FIND_NG(_fi, _fl, _msg)	\
  do {							\
    const struct flow *__fl;				\
    __fl = (_fi)->find_func((_fi), (_fl));		\
    TEST_ASSERT_FALSE_MESSAGE(__fl == (_fl), (_msg));	\
  } while (0)

/* Positively assert an instruction of the given type. */
#define TEST_ASSERT_INSNL_HAS_INSN(_insn, _type, _il, _entry)	\
  do {								\
    _IL_FIND_INSN(_insn, _type, _il, _entry);			\
    TEST_ASSERT_NOT_NULL(_insn);				\
  } while (0)

/* Negatively assert an instruction of the given type. */
#define TEST_ASSERT_INSNL_NO_INSN(_type, _il, _entry)	\
  do {							\
    struct instruction *_insn = NULL;			\
    _IL_FIND_INSN(_insn, _type, _il, _entry);		\
    TEST_ASSERT_NULL(_insn);				\
  } while (0)

#ifdef unused
/* Assert an empty match list. */
#define TEST_ASSERT_MATCH_EMPTY(_ml)			\
  do {							\
    TEST_ASSERT_EQUAL_INT(0, TAILQ_COUNT(_ml));		\
  } while (0)
#endif /* unused */

/* Assert a flow count. */
#define TEST_ASSERT_TABLE_NFLOW(_tp, _idx, _expected)	\
  do {							\
    TEST_ASSERT_NOT_NULL_MESSAGE(*(_tp),				\
                                 "table_get: table not found");		\
    TEST_ASSERT_EQUAL_MESSAGE((*(_tp))->flow_list->nflow, (_expected),  \
                              "entry count error");			\
  } while (0)

/* Assert a valid VLAN VID for OpenFlow, including the presence bit. */
#define TEST_ASSERT_VLAN_VID(_vid)					\
  do {									\
    TEST_ASSERT_EQUAL_INT(0,						\
                          ((uint16_t)((_vid) & ~OFPVID_PRESENT)) >> /* VLAN_BITS */12); \
  } while (0)


#endif /* __DATAPLANE_TEST_MISC_MACROS_H__ */
