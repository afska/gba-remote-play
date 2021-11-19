#!/bin/bash

cd "$(dirname "$0")"

function upload {
  ./multiboot.tool gba.mb.gba
}

upload
while [ $? -ne 0 ]; do
  upload
done

sudo ./raspi.run
