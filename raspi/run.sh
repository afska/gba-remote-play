#!/bin/bash

OUTPUT="out/raspi.run"

rm -f "$OUTPUT"
g++ -pthread -lpigpio -lrt ./src/** -o "$OUTPUT"
"$OUTPUT"
