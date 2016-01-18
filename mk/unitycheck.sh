#!/bin/sh

RUBY=ruby
CC=$1
UNITY_DIR=tools/unity

if [ -z "$CC" ]; then
  CC=cc
fi

cat<<EOF>/tmp/test$$.c
#include "unity.h"
void setUp (void) {}
void tearDown (void) {}
void test_Test1 (void) { TEST_ASSERT_EQUAL(0, 0); }
EOF
${RUBY} "${UNITY_DIR}/auto/generate_test_runner.rb" \
    /tmp/test$$.c /tmp/test_runner$$.c > /dev/null 2>&1 && \
${CC} "-I${UNITY_DIR}/src" /tmp/test$$.c /tmp/test_runner$$.c \
    "${UNITY_DIR}/src/unity.c" -o /tmp/test$$.out && \
/tmp/test$$.out > /tmp/test$$.result
ret=$?
rm -f /tmp/test$$.c /tmp/test_runner$$.c /tmp/test$$.out /tmp/test$$.result
exit $ret
