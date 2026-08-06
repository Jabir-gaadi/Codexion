#!/bin/bash

EXPECTED=$((200 * 60))

echo "Running..."

OUT=$(./codexion 200 2000 20 20 20 60 200 fifo)

COMPILE=$(echo "$OUT" | grep -c "is compiling")
DEBUG=$(echo "$OUT" | grep -c "is debugging")
REFACTOR=$(echo "$OUT" | grep -c "is refactoring")
BURNOUT=$(echo "$OUT" | grep -c "burned out")

echo "Compile : $COMPILE / $EXPECTED"
echo "Debug   : $DEBUG / $EXPECTED"
echo "Refactor: $REFACTOR / $EXPECTED"
echo "Burnout : $BURNOUT"