#!/bin/bash

wait_sec=5

set -eux
cat <<EOF | nc -q$wait_sec localhost 4444
reset halt
flash write_image erase nucleo.bin 0x08000000
reset run
EOF
