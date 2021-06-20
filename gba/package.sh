#!/bin/bash

# // TODO: REMOVE THIS FILE AND GBFS

FILE_INPUT="gba.mb.gba"
FILE_TMP="gba.mb.tmp.gba"
FILE_OUTPUT="gba.mb.out.gba"
DATA="files.gbfs"

if [ ! -f "$DATA" ]; then
    echo ""
    echo "The file $DATA does not exist. Run make import first!"
    echo ""
    exit 1
fi

KB=$((1024))
MAX_ROM_SIZE_KB=$((256 - 1))
INITIAL_REQUIRED_SIZE_KB=1024

ROM_SIZE=$(wc -c < $FILE_INPUT)
GBFS_SIZE=$(wc -c < $DATA)
ROM_SIZE_KB=$(($ROM_SIZE / $KB))
GBFS_SIZE_KB=$(($GBFS_SIZE / $KB))
MAX_REQUIRED_SIZE_KB=$(($MAX_ROM_SIZE_KB - $GBFS_SIZE_KB))
if (( $MAX_REQUIRED_SIZE_KB < $ROM_SIZE_KB )); then
    echo ""
    echo "[!] ERROR:"
    echo "GBFS file too big."
    echo ""
    echo "GBFS_SIZE_KB=$GBFS_SIZE_KB"
    echo "ROM_SIZE_KB=$ROM_SIZE_KB"
    echo "(MAX_ROM_SIZE_KB=$MAX_ROM_SIZE_KB)"
    echo ""
    exit 1
fi
REQUIRED_SIZE_KB=$(($INITIAL_REQUIRED_SIZE_KB > $MAX_REQUIRED_SIZE_KB ? $MAX_REQUIRED_SIZE_KB : $INITIAL_REQUIRED_SIZE_KB))
PAD_NEEDED=$((($REQUIRED_SIZE_KB * $KB) - $ROM_SIZE))

cp $FILE_INPUT $FILE_TMP
dd if=/dev/zero bs=1 count=$PAD_NEEDED >> $FILE_TMP
cat $FILE_TMP $DATA > $FILE_OUTPUT
rm $FILE_TMP
