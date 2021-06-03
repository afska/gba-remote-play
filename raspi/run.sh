#!/bin/bash

OUTPUT="out/raspi.run"

rm -f "$OUTPUT"

g++ \
  -Ofast \
  -I./lib/include \
  -I/opt/vc/include \
  -I/opt/vc/include/interface/vcos/pthreads \
  -I/opt/vc/include/interface/vmcs_host \
  -I/opt/vc/include/interface/vmcs_host/linux \
  -L./lib \
  -L/opt/vc/lib \
  -lbcm_host \
  ./src/** \
  ./lib/libbcm2835.a \
  ./lib/libimagequant.a \
  -o "$OUTPUT"

"$OUTPUT"
