#!/bin/bash

OUTPUT="out/raspi.run"

rm -f "$OUTPUT"
g++ -pthread -lpigpio -lrt -Ofast ./src/** -o "$OUTPUT"
"$OUTPUT"
