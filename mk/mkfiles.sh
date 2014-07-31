#!/bin/sh

TARGET=./.files
rm -rf ${TARGET}
echo 'INPUT = \' >> ${TARGET}
find . -type f -name '*.c' -o -name '*.cpp' -o -name '*.h' | \
    awk '{ printf "%s \\\n", $1 }' | \
    grep -v 'check/' >> ${TARGET}
echo >> ${TARGET}
