#!/bin/bash

OUTPUT="out/raspi.run"

rm -f "$OUTPUT"
g++ -lwiringPi ./src/** -o "$OUTPUT"
"$OUTPUT"
