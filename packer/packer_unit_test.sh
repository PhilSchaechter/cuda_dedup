#!/bin/bash 

set -x
set -e
#gcc -g ../system/is_logger.c ../store/store.c packer.c packer_unit_test.c -I. -I../store -I../system -L/usr/local/lib -lzlog -lpthread
bazel build  //packer:unit-test
dd if=/dev/zero of=/tmp/loopbackfile.img bs=1M count=1000
du -sh /tmp/loopbackfile.img
losetup -fP /tmp/loopbackfile.img
losetup -a
time ../bazel-bin/packer/unit-test
time ../bazel-bin/packer/unit-test
losetup -d /dev/loop0
rm -f /tmp/loopbackfile.img
rm -f ../bazel-bin/packer/unit-test


