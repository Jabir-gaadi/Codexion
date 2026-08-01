# #!/bin/bash

# EXPECTED=$((200 * 60))

# echo "Running..."

# OUT=$(./codexion 200 2000 20 20 20 60 200 fifo)

# COMPILE=$(echo "$OUT" | grep -c "is compiling")
# DEBUG=$(echo "$OUT" | grep -c "is debugging")
# REFACTOR=$(echo "$OUT" | grep -c "is refactoring")
# BURNOUT=$(echo "$OUT" | grep -c "burned out")

# echo "Compile : $COMPILE / $EXPECTED"
# echo "Debug   : $DEBUG / $EXPECTED"
# echo "Refactor: $REFACTOR / $EXPECTED"
# echo "Burnout : $BURNOUT"

#!/bin/bash

CODERS=200
REQUIRED=60
TIMEOUT=120
LOG="codexion_stress.log"

echo "Running..."

timeout "${TIMEOUT}s" \
	./codexion "$CODERS" 2000 20 20 20 "$REQUIRED" 200 fifo \
	> "$LOG"

STATUS=$?

if [ "$STATUS" -eq 124 ]; then
	echo "FAIL: Program hung for more than ${TIMEOUT}s"
	exit 1
fi

if [ "$STATUS" -ne 0 ]; then
	echo "FAIL: Program exited with status $STATUS"
	exit 1
fi

COMPILE=$(grep -c "is compiling" "$LOG")
DEBUG=$(grep -c "is debugging" "$LOG")
REFACTOR=$(grep -c "is refactoring" "$LOG")
BURNOUT=$(grep -c "burned out" "$LOG")

echo "Compile : $COMPILE"
echo "Debug   : $DEBUG"
echo "Refactor: $REFACTOR"
echo "Burnout : $BURNOUT"

FAILED=0

if [ "$BURNOUT" -ne 0 ]; then
	echo "FAIL: A coder burned out"
	grep "burned out" "$LOG"
	FAILED=1
fi

echo
echo "Checking every coder..."

awk -v coders="$CODERS" -v required="$REQUIRED" '
$3 == "is" && $4 == "compiling" {
	count[$2]++
}
END {
	failed = 0

	for (id = 1; id <= coders; id++) {
		if (count[id] < required) {
			printf "FAIL: Coder %d compiled %d/%d times\n",
				id, count[id] + 0, required
			failed = 1
		}
	}

	exit failed
}
' "$LOG"

if [ "$?" -ne 0 ]; then
	FAILED=1
fi

if [ "$FAILED" -eq 0 ]; then
	echo "PASS: Every coder compiled at least $REQUIRED times"
	exit 0
fi

echo "FAIL: Stress test failed"
exit 1