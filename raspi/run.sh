#!/bin/bash

OUTPUT="out/raspi.run"

rm -f "$OUTPUT"
g++ ./src/** -o "$OUTPUT"
"$OUTPUT"
