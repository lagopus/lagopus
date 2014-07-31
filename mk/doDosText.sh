#!/bin/sh

l=`find . -type f \( \
    -name '*.h' -o \
    -name '*.c' -o \
    -name '*.cpp' -o \
    -name 'Makefile.in' \)`

for i in ${l}; do
    nkf -c ${i} > ${i}.tmp
    rm -f ${i}
    mv -f ${i}.tmp ${i}
done

exit 0


