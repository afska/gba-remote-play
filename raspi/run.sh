#!/bin/bash

OUTPUT="out/raspi.run"

rm -f "$OUTPUT"
g++ -lwiringPi -Ofast ./src/** -o "$OUTPUT"
"$OUTPUT"
