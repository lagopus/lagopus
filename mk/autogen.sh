#!/bin/sh

aclocal
libtoolize -c -f -i
autoconf

rm -rf autom4te.cache

exit 0

