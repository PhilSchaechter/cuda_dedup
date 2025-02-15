#!/bin/bash

set -x
set -e
#gcc -g ../system/is_logger.c index.c index_unit_test.c -I. -I../store -I../system -L/usr/local/lib -lzlog -lpthread
bazel build //index:unit-test
../bazel-bin/index/unit-test
rm -f ../bazel-bin/index/unit-test
