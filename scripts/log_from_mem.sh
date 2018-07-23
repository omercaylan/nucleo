#!/bin/bash

set -eu

wait_sec=2
filename=_dump.mem
rm -f $filename
n_bytes=256

(>&2 echo "dumping $n_bytes bytes to file $filename")

set -eux
cat <<EOF | nc -q$wait_sec localhost 4444
dump_image $filename 0x20000000 $n_bytes
EOF

cat $filename | tr '\0' '\n'
#xxd $filename
