#!/bin/bash

SOURCE="src/data"
DESTINATION="_compiled_files"

cd "$SOURCE"

# [Setup]
mkdir -p "$DESTINATION"
rm $DESTINATION/*.h $DESTINATION/*.c

# ...

# [Cleanup]
rm *.h *.c
