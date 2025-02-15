#!/bin/bash 

set -x
set -e

#gcc -g ../system/is_logger.c store_unit_test.c store.c -I. -I../packer -I../system -L/usr/local/lib -lzlog -lpthread
bazel build //store:unit-test
dd if=/dev/zero of=/tmp/loopbackfile.img bs=1M count=1000
du -sh /tmp/loopbackfile.img
losetup -fP /tmp/loopbackfile.img
losetup -a
../bazel-bin/store/unit-test
../bazel-bin/store/unit-test
losetup -d /dev/loop0
rm -f /tmp/loopbackfile.img
rm -f ../bazel-bin/store/unit-test
