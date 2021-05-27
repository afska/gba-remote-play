#!/bin/bash

OUTPUT="out/raspi.run"

rm -f "$OUTPUT"
g++ \
  -Ofast \
  -I/opt/vc/include \
  -I/opt/vc/include/interface/vcos/pthreads \
  -I/opt/vc/include/interface/vmcs_host \
  -I/opt/vc/include/interface/vmcs_host/linux \
  -L/opt/vc/lib \
  -lwiringPi \
  -lbcm_host \
  ./src/** \
  -o "$OUTPUT"
"$OUTPUT"
