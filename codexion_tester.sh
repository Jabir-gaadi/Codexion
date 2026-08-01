#!/usr/bin/env bash

# Codexion black-box tester
# Usage:
#   bash codexion_tester.sh [--bin ./codexion] [--stress 10]
#                              [--no-build] [--valgrind] [--helgrind]

set -u
set -o pipefail

BIN="./codexion"
STRESS_RUNS=5
DO_BUILD=1
DO_VALGRIND=0
DO_HELGRIND=0
PASS_COUNT=0
FAIL_COUNT=0
WARN_COUNT=0

usage()
{
	printf '%s\n' \
		"Usage: $0 [options]" \
		"" \
		"Options:" \
		"  --bin PATH       Path to codexion (default: ./codexion)" \
		"  --stress N       Repeat FIFO and EDF liveness tests N times (default: 5)" \
		"  --no-build       Do not run 'make'" \
		"  --valgrind       Run an optional Valgrind leak test" \
		"  --helgrind       Run an optional Helgrind data-race test" \
		"  -h, --help       Show this help"
}

while [ "$#" -gt 0 ]; do
	case "$1" in
		--bin)
			[ "$#" -ge 2 ] || { usage >&2; exit 2; }
			BIN="$2"
			shift 2
			;;
		--stress)
			[ "$#" -ge 2 ] || { usage >&2; exit 2; }
			STRESS_RUNS="$2"
			shift 2
			;;
		--no-build)
			DO_BUILD=0
			shift
			;;
		--valgrind)
			DO_VALGRIND=1
			shift
			;;
		--helgrind)
			DO_HELGRIND=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			printf 'Unknown option: %s\n' "$1" >&2
			usage >&2
			exit 2
			;;
	esac
done

case "$STRESS_RUNS" in
	''|*[!0-9]*)
		printf '%s\n' "Error: --stress must be a non-negative integer." >&2
		exit 2
		;;
esac

if command -v timeout >/dev/null 2>&1; then
	TIMEOUT_BIN="timeout"
elif command -v gtimeout >/dev/null 2>&1; then
	TIMEOUT_BIN="gtimeout"
else
	printf '%s\n' "Error: install 'timeout' (coreutils) before running this tester." >&2
	exit 2
fi

if [ -t 1 ]; then
	RED=$(printf '\033[31m')
	GREEN=$(printf '\033[32m')
	YELLOW=$(printf '\033[33m')
	BLUE=$(printf '\033[34m')
	RESET=$(printf '\033[0m')
else
	RED=""
	GREEN=""
	YELLOW=""
	BLUE=""
	RESET=""
fi

LOG_DIR="${CODEXION_TEST_LOG_DIR:-codexion_test_logs_$(date +%Y%m%d_%H%M%S)}"
mkdir -p "$LOG_DIR" || exit 2

pass()
{
	PASS_COUNT=$((PASS_COUNT + 1))
	printf '%s[PASS]%s %s\n' "$GREEN" "$RESET" "$1"
}

fail()
{
	FAIL_COUNT=$((FAIL_COUNT + 1))
	printf '%s[FAIL]%s %s\n' "$RED" "$RESET" "$1"
}

warn()
{
	WARN_COUNT=$((WARN_COUNT + 1))
	printf '%s[WARN]%s %s\n' "$YELLOW" "$RESET" "$1"
}

safe_name()
{
	printf '%s' "$1" | tr ' /:' '___' | tr -cd '[:alnum:]_.-'
}

run_program()
{
	local name="$1"
	local seconds="$2"
	shift 2
	local stem

	stem="$(safe_name "$name")"
	LAST_OUT="$LOG_DIR/$stem.out"
	LAST_ERR="$LOG_DIR/$stem.err"
	"$TIMEOUT_BIN" "${seconds}s" "$BIN" "$@" >"$LAST_OUT" 2>"$LAST_ERR"
	LAST_STATUS=$?
	printf 'command: %q' "$BIN" >"$LOG_DIR/$stem.cmd"
	printf ' %q' "$@" >>"$LOG_DIR/$stem.cmd"
	printf '\nexit_status: %d\n' "$LAST_STATUS" >>"$LOG_DIR/$stem.cmd"
}

check_log_format()
{
	local file="$1"
	local coders="$2"

	awk -v max_id="$coders" '
	BEGIN { ok = 1; previous = -1 }
	{
		if ($0 !~ /^[0-9]+ [1-9][0-9]* (has taken a dongle|is compiling|is debugging|is refactoring|burned out)$/) {
			printf "invalid log line %d: %s\n", NR, $0 > "/dev/stderr"
			ok = 0
			next
		}
		if (($2 + 0) < 1 || ($2 + 0) > max_id) {
			printf "coder id out of range on line %d: %s\n", NR, $0 > "/dev/stderr"
			ok = 0
		}
		if (($1 + 0) < previous) {
			printf "timestamp moved backward on line %d: %s\n", NR, $0 > "/dev/stderr"
			ok = 0
		}
		previous = $1 + 0
		if (burned) {
			printf "message printed after burnout on line %d: %s\n", NR, $0 > "/dev/stderr"
			ok = 0
		}
		if ($3 == "burned")
			burned = 1
	}
	END { exit(ok ? 0 : 1) }
	' "$file"
}

check_two_takes_before_compile()
{
	local file="$1"

	awk '
	{
		id = $2 + 0
		if ($3 == "has" && $4 == "taken")
			takes[id]++
		else if ($3 == "is" && $4 == "compiling") {
			if (takes[id] != 2) {
				printf "coder %d compiled after %d dongle logs at line %d\n",
					id, takes[id], NR > "/dev/stderr"
				ok = 0
			}
			takes[id] = 0
		}
	}
	BEGIN { ok = 1 }
	END { exit(ok ? 0 : 1) }
	' "$file"
}

check_completion_counts()
{
	local file="$1"
	local coders="$2"
	local required="$3"

	awk -v coders="$coders" -v required="$required" '
	$3 == "is" && $4 == "compiling" { count[$2]++ }
	$3 == "burned" { burned = 1 }
	END {
		ok = !burned
		for (i = 1; i <= coders; i++) {
			if (count[i] < required) {
				printf "coder %d compiled %d time(s), expected at least %d\n",
					i, count[i], required > "/dev/stderr"
				ok = 0
			}
		}
		exit(ok ? 0 : 1)
	}
	' "$file"
}

report_overshoot()
{
	local file="$1"
	local coders="$2"
	local required="$3"

	awk -v coders="$coders" -v required="$required" '
	$3 == "is" && $4 == "compiling" { count[$2]++ }
	END {
		for (i = 1; i <= coders; i++)
			if (count[i] > required)
				printf "  note: coder %d compiled %d times (target was %d; subject permits at least %d)\n",
					i, count[i], required, required
	}
	' "$file"
}

check_burnout_precision()
{
	local file="$1"
	local burnout_ms="$2"

	awk -v limit="$burnout_ms" '
	$3 == "is" && $4 == "compiling" { last_start[$2] = $1 + 0 }
	$3 == "burned" {
		deaths++
		id = $2 + 0
		actual_log = $1 + 0
		base = (id in last_start) ? last_start[id] : 0
		expected = base + limit
		delay = actual_log - expected
		if (delay < 0 || delay > 10) {
			printf "burnout delay is %d ms (expected timestamp %d..%d, got %d)\n",
				delay, expected, expected + 10, actual_log > "/dev/stderr"
			ok = 0
		}
	}
	BEGIN { ok = 1 }
	END {
		if (deaths != 1) {
			printf "expected exactly one burnout log, got %d\n", deaths > "/dev/stderr"
			ok = 0
		}
		exit(ok ? 0 : 1)
	}
	' "$file"
}

check_cooldown_spacing()
{
	local file="$1"
	local minimum="$2"

	awk -v minimum="$minimum" '
	$3 == "is" && $4 == "compiling" {
		now = $1 + 0
		if (seen && now - previous < minimum) {
			printf "compile starts only %d ms apart; expected at least %d ms\n",
				now - previous, minimum > "/dev/stderr"
			ok = 0
		}
		previous = now
		seen = 1
	}
	BEGIN { ok = 1 }
	END { exit(ok ? 0 : 1) }
	' "$file"
}

valid_completion_test()
{
	local name="$1"
	local scheduler="$2"
	local coders="$3"
	local required="$4"
	local repeats="$5"
	local i

	i=1
	while [ "$i" -le "$repeats" ]; do
		run_program "${name}_${i}" 8 \
			"$coders" 2000 50 20 20 "$required" 5 "$scheduler"
		if [ "$LAST_STATUS" -ne 0 ]; then
			fail "$name run $i exited with status $LAST_STATUS"
			i=$((i + 1))
			continue
		fi
		if ! check_log_format "$LAST_OUT" "$coders" 2>>"$LAST_ERR"; then
			fail "$name run $i produced invalid or non-serialized logs"
		elif ! check_two_takes_before_compile "$LAST_OUT" 2>>"$LAST_ERR"; then
			fail "$name run $i violated the two-dongle lifecycle"
		elif ! check_completion_counts "$LAST_OUT" "$coders" "$required" 2>>"$LAST_ERR"; then
			fail "$name run $i did not complete safely"
		else
			pass "$name run $i completed safely"
			report_overshoot "$LAST_OUT" "$coders" "$required"
		fi
		i=$((i + 1))
	done
}

printf '%sCodexion test suite%s\n' "$BLUE" "$RESET"
printf 'Binary: %s\nLogs:   %s\n\n' "$BIN" "$LOG_DIR"

if [ "$DO_BUILD" -eq 1 ]; then
	if [ ! -f Makefile ]; then
		fail "Makefile is missing from the current directory"
	else
		if make >"$LOG_DIR/build.out" 2>"$LOG_DIR/build.err"; then
			pass "make"
			if grep -Eq -- '-Wall([[:space:]]|$)' "$LOG_DIR/build.out" \
				&& grep -Eq -- '-Wextra([[:space:]]|$)' "$LOG_DIR/build.out" \
				&& grep -Eq -- '-Werror([[:space:]]|$)' "$LOG_DIR/build.out" \
				&& grep -Eq -- '-pthread([[:space:]]|$)' "$LOG_DIR/build.out"; then
				pass "required compiler flags visible in build output"
			else
				warn "could not verify -Wall -Wextra -Werror -pthread in build output"
			fi
		else
			fail "make"
		fi
	fi
fi

if [ ! -x "$BIN" ]; then
	fail "binary is missing or not executable: $BIN"
	printf '\nFix the build/binary path and run the tester again.\n'
	exit 1
fi

printf '\n%sArgument validation%s\n' "$BLUE" "$RESET"

run_program "missing_arguments" 2
if [ "$LAST_STATUS" -ne 0 ] && [ "$LAST_STATUS" -ne 124 ]; then
	pass "reject missing arguments"
else
	fail "reject missing arguments (status $LAST_STATUS)"
fi

run_program "extra_argument" 2 2 1000 50 20 20 1 0 fifo extra
if [ "$LAST_STATUS" -ne 0 ] && [ "$LAST_STATUS" -ne 124 ]; then
	pass "reject extra argument"
else
	fail "reject extra argument (status $LAST_STATUS)"
fi

run_program "zero_coders" 2 0 1000 50 20 20 1 0 fifo
if [ "$LAST_STATUS" -ne 0 ] && [ "$LAST_STATUS" -ne 124 ]; then
	pass "reject zero coders"
else
	fail "reject zero coders (status $LAST_STATUS)"
fi

VALID=(2 1000 50 20 20 1 0 fifo)
NUMERIC_NAMES=(number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown)
i=0
while [ "$i" -lt 7 ]; do
	ARGS=("${VALID[@]}")
	ARGS[$i]="abc"
	run_program "nonnumeric_${NUMERIC_NAMES[$i]}" 2 "${ARGS[@]}"
	if [ "$LAST_STATUS" -ne 0 ] && [ "$LAST_STATUS" -ne 124 ]; then
		pass "reject non-numeric ${NUMERIC_NAMES[$i]}"
	else
		fail "reject non-numeric ${NUMERIC_NAMES[$i]} (status $LAST_STATUS)"
	fi

	ARGS=("${VALID[@]}")
	ARGS[$i]="-1"
	run_program "negative_${NUMERIC_NAMES[$i]}" 2 "${ARGS[@]}"
	if [ "$LAST_STATUS" -ne 0 ] && [ "$LAST_STATUS" -ne 124 ]; then
		pass "reject negative ${NUMERIC_NAMES[$i]}"
	else
		fail "reject negative ${NUMERIC_NAMES[$i]} (status $LAST_STATUS)"
	fi

	ARGS=("${VALID[@]}")
	ARGS[$i]="9223372036854775808"
	run_program "overflow_${NUMERIC_NAMES[$i]}" 2 "${ARGS[@]}"
	if [ "$LAST_STATUS" -ne 0 ] && [ "$LAST_STATUS" -ne 124 ]; then
		pass "reject overflow in ${NUMERIC_NAMES[$i]}"
	else
		fail "reject overflow in ${NUMERIC_NAMES[$i]} (status $LAST_STATUS)"
	fi
	i=$((i + 1))
done

for invalid_scheduler in EDF FIFO random edf1 ""; do
	run_program "invalid_scheduler_${invalid_scheduler:-empty}" 2 \
		2 1000 50 20 20 1 0 "$invalid_scheduler"
	if [ "$LAST_STATUS" -ne 0 ] && [ "$LAST_STATUS" -ne 124 ]; then
		pass "reject scheduler '${invalid_scheduler:-<empty>}'"
	else
		fail "reject scheduler '${invalid_scheduler:-<empty>}' (status $LAST_STATUS)"
	fi
done

printf '\n%sRuntime and logging%s\n' "$BLUE" "$RESET"

run_program "single_coder_burnout" 3 1 120 60 20 20 1 0 fifo
if [ "$LAST_STATUS" -ne 0 ]; then
	fail "single-coder test exited with status $LAST_STATUS"
elif ! check_log_format "$LAST_OUT" 1 2>>"$LAST_ERR"; then
	fail "single-coder test produced invalid logs"
elif ! check_burnout_precision "$LAST_OUT" 120 2>>"$LAST_ERR"; then
	fail "single-coder burnout was not logged within 10 ms"
else
	pass "single coder takes one dongle and burns out within 10 ms"
fi

run_program "forced_burnout" 4 2 150 100 0 0 3 0 fifo
if [ "$LAST_STATUS" -ne 0 ]; then
	fail "forced-burnout test exited with status $LAST_STATUS"
elif ! check_log_format "$LAST_OUT" 2 2>>"$LAST_ERR"; then
	fail "forced-burnout test produced invalid logs"
elif ! check_two_takes_before_compile "$LAST_OUT" 2>>"$LAST_ERR"; then
	fail "forced-burnout test violated the two-dongle lifecycle"
elif ! check_burnout_precision "$LAST_OUT" 150 2>>"$LAST_ERR"; then
	fail "forced burnout was not logged within 10 ms of its deadline"
else
	pass "forced burnout timing and termination"
fi

run_program "cooldown" 6 2 2000 50 0 0 2 70 fifo
if [ "$LAST_STATUS" -ne 0 ]; then
	fail "cooldown test exited with status $LAST_STATUS"
elif ! check_log_format "$LAST_OUT" 2 2>>"$LAST_ERR"; then
	fail "cooldown test produced invalid logs"
elif ! check_completion_counts "$LAST_OUT" 2 2 2>>"$LAST_ERR"; then
	fail "cooldown test did not complete safely"
elif ! check_cooldown_spacing "$LAST_OUT" 118 2>>"$LAST_ERR"; then
	fail "dongle cooldown was not respected"
else
	pass "dongle cooldown (50 ms compile + 70 ms cooldown)"
fi

printf '\n%sFIFO/EDF liveness stress%s\n' "$BLUE" "$RESET"
if [ "$STRESS_RUNS" -eq 0 ]; then
	warn "stress tests disabled (--stress 0)"
else
	valid_completion_test "fifo_liveness" fifo 5 3 "$STRESS_RUNS"
	valid_completion_test "edf_liveness" edf 5 3 "$STRESS_RUNS"
fi

if [ "$DO_VALGRIND" -eq 1 ]; then
	printf '\n%sValgrind%s\n' "$BLUE" "$RESET"
	if ! command -v valgrind >/dev/null 2>&1; then
		warn "Valgrind is not installed"
	else
		"$TIMEOUT_BIN" 20s valgrind \
			--leak-check=full \
			--show-leak-kinds=all \
			--errors-for-leak-kinds=all \
			--error-exitcode=99 \
			"$BIN" 2 1000 30 10 10 2 0 fifo \
			>"$LOG_DIR/valgrind.out" 2>"$LOG_DIR/valgrind.err"
		status=$?
		if [ "$status" -eq 0 ]; then
			pass "Valgrind leak/error check"
		else
			fail "Valgrind reported an error or timeout (status $status)"
		fi
	fi
fi

if [ "$DO_HELGRIND" -eq 1 ]; then
	printf '\n%sHelgrind%s\n' "$BLUE" "$RESET"
	if ! command -v valgrind >/dev/null 2>&1; then
		warn "Valgrind/Helgrind is not installed"
	else
		"$TIMEOUT_BIN" 30s valgrind \
			--tool=helgrind \
			--error-exitcode=98 \
			"$BIN" 4 2000 30 10 10 2 0 edf \
			>"$LOG_DIR/helgrind.out" 2>"$LOG_DIR/helgrind.err"
		status=$?
		if [ "$status" -eq 0 ]; then
			pass "Helgrind data-race check"
		else
			fail "Helgrind reported an error or timeout (status $status)"
		fi
	fi
fi

printf '\n%sSummary%s\n' "$BLUE" "$RESET"
printf 'Passed:   %d\nFailed:   %d\nWarnings: %d\nLogs:     %s\n' \
	"$PASS_COUNT" "$FAIL_COUNT" "$WARN_COUNT" "$LOG_DIR"

if [ "$FAIL_COUNT" -ne 0 ]; then
	exit 1
fi
exit 0
