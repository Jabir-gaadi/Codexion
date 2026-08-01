#!/bin/bash

# ============================================================
# Codexion Test Suite — Full Stress & Edge Case Battery
# Usage: bash test_codexion.sh [path_to_binary]
# ============================================================

BIN=${1:-./codexion}
PASS=0
FAIL=0
TOTAL=0
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

TIMEOUT=10  # seconds before declaring deadlock

section()
{
    echo ""
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${CYAN}  $1${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
}

check()
{
    local label="$1"
    local ok="$2"
    ((TOTAL++))
    if [ "$ok" = "1" ]; then
        echo -e "    ${GREEN}[PASS]${NC} $label"
        ((PASS++))
    else
        echo -e "    ${RED}[FAIL]${NC} $label"
        ((FAIL++))
    fi
}

run_test()
{
    local desc="$1"
    local args="$2"
    local expect_burnout="$3"
    local expect_compiles="$4"
    local extra_checks="$5"

    echo -e "\n${YELLOW}▶ $desc${NC}"
    echo -e "  ${BLUE}$BIN $args${NC}"

    output=$(timeout $TIMEOUT $BIN $args 2>&1)
    exit_code=$?

    burned=$(echo "$output"   | grep -c "burned out")
    compiled=$(echo "$output" | grep -c "is compiling")
    timedout=0
    [ "$exit_code" -eq 124 ] && timedout=1

    # deadlock / hang check
    check "program exits within ${TIMEOUT}s (no deadlock)" \
        "$([ $timedout -eq 0 ] && echo 1 || echo 0)"

    # burnout expectation
    if [ "$expect_burnout" = "yes" ]; then
        check "burnout detected" \
            "$([ $burned -ge 1 ] && echo 1 || echo 0)"
        # burnout timestamp must be > 0
        bt=$(echo "$output" | grep "burned out" | awk '{print $1}' | head -1)
        check "burnout timestamp > 0" \
            "$([ -n "$bt" ] && [ "$bt" -gt 0 ] && echo 1 || echo 0)"
    elif [ "$expect_burnout" = "no" ]; then
        check "no burnout" \
            "$([ $burned -eq 0 ] && echo 1 || echo 0)"
    fi

    # compile count
    if [ -n "$expect_compiles" ] && [ "$expect_burnout" = "no" ]; then
        nb=$(echo "$args" | awk '{print $1}')
        expected=$((nb * expect_compiles))
        check "compile count = $nb × $expect_compiles = $expected (got $compiled)" \
            "$([ $compiled -eq $expected ] && echo 1 || echo 0)"
    fi

    # only one burned out line
    if [ "$expect_burnout" = "yes" ]; then
        check "exactly 1 burned out line" \
            "$([ $burned -eq 1 ] && echo 1 || echo 0)"
    fi

    # output ordering: timestamps must be non-decreasing
    ts_ok=1
    prev=0
    while IFS= read -r line; do
        ts=$(echo "$line" | awk '{print $1}')
        if [[ "$ts" =~ ^[0-9]+$ ]]; then
            if [ "$ts" -lt "$prev" ]; then
                ts_ok=0
                break
            fi
            prev=$ts
        fi
    done <<< "$output"
    check "timestamps non-decreasing" "$ts_ok"

    # extra caller-supplied checks
    if [ -n "$extra_checks" ]; then
        eval "$extra_checks"
    fi
}

# ─────────────────────────────────────────────────
# INVALID ARGS — program must reject and not hang
# ─────────────────────────────────────────────────
section "INVALID INPUT"

check_invalid()
{
    local desc="$1"
    local args="$2"
    echo -e "\n${YELLOW}▶ $desc${NC}"
    echo -e "  ${BLUE}$BIN $args${NC}"
    output=$(timeout 3 $BIN $args 2>&1)
    ec=$?
    ((TOTAL++))
    if [ "$ec" -ne 0 ] && [ "$ec" -ne 124 ]; then
        echo -e "    ${GREEN}[PASS]${NC} rejected with non-zero exit"
        ((PASS++))
    else
        echo -e "    ${RED}[FAIL]${NC} should have rejected (exit=$ec)"
        ((FAIL++))
    fi
}

check_invalid "no arguments"          ""
check_invalid "too few arguments"     "4 400 200"
check_invalid "negative coders"       "-1 400 200 100 100 2 0 fifo"
check_invalid "negative burnout"      "4 -400 200 100 100 2 0 fifo"
check_invalid "zero coders"           "0 400 200 100 100 2 0 fifo"
check_invalid "invalid scheduler"     "4 400 200 100 100 2 0 random"
check_invalid "scheduler missing"     "4 400 200 100 100 2 0"
check_invalid "non-integer burnout"   "4 abc 200 100 100 2 0 fifo"
check_invalid "float argument"        "4 400.5 200 100 100 2 0 fifo"

# ─────────────────────────────────────────────────
# SINGLE CODER — must always burn out (only 1 dongle)
# ─────────────────────────────────────────────────
section "SINGLE CODER (always burns out)"

run_test "1 coder, burnout=400, FIFO" \
    "1 400 200 100 100 2 0 fifo" "yes" ""

run_test "1 coder, burnout=800, FIFO" \
    "1 800 100 50 50 2 0 fifo" "yes" ""

run_test "1 coder, burnout=800, EDF" \
    "1 800 100 50 50 2 0 edf" "yes" ""

# ─────────────────────────────────────────────────
# SHOULD NOT BURN OUT — generous timing
# ─────────────────────────────────────────────────
section "SHOULD NOT BURN OUT"

run_test "2 coders, generous burnout, FIFO" \
    "2 800 100 50 50 2 0 fifo" "no" "2"

run_test "2 coders, generous burnout, EDF" \
    "2 800 100 50 50 2 0 edf" "no" "2"

run_test "3 coders, generous burnout, FIFO" \
    "3 800 100 50 50 2 0 fifo" "no" "2"

run_test "4 coders, generous burnout, FIFO" \
    "4 800 100 50 50 2 0 fifo" "no" "2"

run_test "4 coders, generous burnout, EDF" \
    "4 800 100 50 50 2 0 edf" "no" "2"

run_test "5 coders, generous burnout, FIFO" \
    "5 1000 100 50 50 2 0 fifo" "no" "2"

run_test "2 coders, 1 compile only, FIFO" \
    "2 600 100 50 50 1 0 fifo" "no" "1"

run_test "4 coders, 5 compiles, EDF" \
    "4 1500 100 50 50 5 0 edf" "no" "5"

run_test "2 coders, zero cooldown, EDF" \
    "2 800 100 50 50 3 0 edf" "no" "3"

run_test "4 coders, small cooldown, FIFO" \
    "4 1000 100 50 50 3 10 fifo" "no" "3"

# ─────────────────────────────────────────────────
# SHOULD BURN OUT — tight timing
# ─────────────────────────────────────────────────
section "SHOULD BURN OUT"

run_test "2 coders, burnout < compile, FIFO" \
    "2 50 200 100 100 2 0 fifo" "yes" ""

run_test "4 coders, burnout = compile, FIFO" \
    "4 200 200 100 100 5 0 fifo" "yes" ""

run_test "4 coders, tight burnout + cooldown, FIFO" \
    "4 400 200 100 100 5 100 fifo" "yes" ""

run_test "4 coders, burnout shorter than cycle, EDF" \
    "4 300 200 100 100 5 50 edf" "yes" ""

run_test "3 coders, very short burnout, FIFO" \
    "3 50 200 100 100 2 0 fifo" "yes" ""

run_test "5 coders, burnout tight, EDF" \
    "5 300 200 100 100 5 50 edf" "yes" ""

# ─────────────────────────────────────────────────
# STRESS — many compiles, long runs
# ─────────────────────────────────────────────────
section "STRESS — many compiles"

run_test "2 coders, 10 compiles, FIFO" \
    "2 2000 100 50 50 10 0 fifo" "no" "10"

run_test "4 coders, 10 compiles, EDF" \
    "4 2000 100 50 50 10 0 edf" "no" "10"

run_test "2 coders, 10 compiles, EDF, with cooldown" \
    "2 3000 100 50 50 10 20 edf" "no" "10"

# ─────────────────────────────────────────────────
# SCHEDULER COMPARISON — same args, both schedulers
# ─────────────────────────────────────────────────
section "FIFO vs EDF same args"

run_test "4 coders generous, FIFO" \
    "4 1000 100 50 50 3 0 fifo" "no" "3"

run_test "4 coders generous, EDF" \
    "4 1000 100 50 50 3 0 edf" "no" "3"

run_test "3 coders tight, FIFO" \
    "3 350 200 50 50 5 0 fifo" "yes" ""

run_test "3 coders tight, EDF" \
    "3 350 200 50 50 5 0 edf" "yes" ""

# ─────────────────────────────────────────────────

# ─────────────────────────────────────────────────
# DONGLE COOLDOWN — dedicated tests
# ─────────────────────────────────────────────────
section "DONGLE COOLDOWN"

# cooldown = 0, should finish fine
run_test "2 coders, zero cooldown, FIFO" \
    "2 800 100 50 50 3 0 fifo" "no" "3"

# cooldown small but nonzero — should still finish with generous burnout
run_test "2 coders, cooldown=50, FIFO" \
    "2 1000 100 50 50 3 50 fifo" "no" "3"

# cooldown=50, EDF
run_test "2 coders, cooldown=50, EDF" \
    "2 1000 100 50 50 3 50 edf" "no" "3"

# cooldown larger than compile time — coder must wait on cooldown, not just is_taken
run_test "2 coders, cooldown > compile, FIFO" \
    "2 1500 100 50 50 2 200 fifo" "no" "2"

# cooldown larger than compile time, EDF
run_test "2 coders, cooldown > compile, EDF" \
    "2 1500 100 50 50 2 200 edf" "no" "2"

# cooldown so large that burnout fires before dongle becomes available
run_test "2 coders, cooldown causes burnout, FIFO" \
    "2 300 100 50 50 5 400 fifo" "yes" ""

# cooldown causes burnout, EDF
run_test "2 coders, cooldown causes burnout, EDF" \
    "2 300 100 50 50 5 400 edf" "yes" ""

# 4 coders, heavy cooldown, generous burnout — should survive
run_test "4 coders, cooldown=100, generous burnout, FIFO" \
    "4 1500 100 50 50 2 100 fifo" "no" "2"

# 4 coders, cooldown=100, tight burnout — should burn out
run_test "4 coders, cooldown=100, tight burnout, FIFO" \
    "4 400 100 50 50 5 100 fifo" "yes" ""

# cooldown exactly equal to burnout — on the edge
run_test "2 coders, cooldown = burnout, FIFO" \
    "2 200 100 50 50 5 200 fifo" "yes" ""

# ─────────────────────────────────────────────────
# HARD CASES — race conditions, timing traps, EDF traps
# ─────────────────────────────────────────────────
section "HARD CASES"

# ── Timing traps ──────────────────────────────────

# burnout = compile + debug + refactor exactly
# coder finishes cycle in time but has 0ms margin on next attempt
run_test "burnout = full cycle time, FIFO" \
    "2 300 100 100 100 3 0 fifo" "no" "3"

# burnout = full cycle - 1ms → must burn out
run_test "burnout = full cycle - 1ms, FIFO" \
    "2 299 100 100 100 3 0 fifo" "yes" ""

# burnout exactly long enough for 2 coders alternating on 1 pair
run_test "burnout tight alternation 2 coders, FIFO" \
    "2 400 200 0 0 3 0 fifo" "no" "3"

# ── Queue ordering traps ──────────────────────────

# many compiles required, tight burnout — EDF should prioritize dying coders
run_test "EDF prioritizes closest deadline, 4 coders" \
    "4 600 100 50 50 5 0 edf" "no" "5"

# same but FIFO — may fail if FIFO doesn't rescue starving coders fast enough
run_test "FIFO same params as above, 4 coders" \
    "4 600 100 50 50 5 0 fifo" "no" "5"

# EDF with unequal deadlines — coders that started later have tighter deadlines
# burnout tight enough that EDF order matters for survival
run_test "EDF deadline ordering matters, 4 coders" \
    "4 500 150 50 50 4 0 edf" "no" "4"

# ── Cooldown traps ────────────────────────────────

# cooldown = compile time → when coder finishes and releases,
# neighbor must wait exactly compile_time before grabbing
# burnout must be > compile + cooldown + compile to survive
run_test "cooldown = compile, must survive, FIFO" \
    "2 600 100 50 50 2 100 fifo" "no" "2"

# cooldown = compile, burnout = compile + cooldown → exactly dies
run_test "cooldown = compile, burnout = compile + cooldown, FIFO" \
    "2 200 100 50 50 5 100 fifo" "yes" ""

# large cooldown, many coders — all waiting stacks up
run_test "5 coders, large cooldown, EDF" \
    "5 2000 100 50 50 3 150 edf" "no" "3"

# ── Starvation traps ──────────────────────────────

# classic starvation: even coders start late, odd ones hog resources
# EDF should rescue even coders by their earlier deadline
run_test "EDF rescues starving coders, 4 coders" \
    "4 700 200 100 100 3 0 edf" "no" "3"

# same but FIFO — starvation more likely since FIFO ignores deadlines
run_test "FIFO starvation risk, 4 coders, tight burnout" \
    "4 500 200 100 100 3 0 fifo" "yes" ""

# ── Single compile traps ──────────────────────────

# 1 compile required — program must exit immediately after all compile once
run_test "1 compile required, 4 coders, FIFO" \
    "4 800 100 50 50 1 0 fifo" "no" "1"

run_test "1 compile required, 4 coders, EDF" \
    "4 800 100 50 50 1 0 edf" "no" "1"

# ── Large coder counts ────────────────────────────

# 7 coders — odd number, asymmetric ring, harder scheduling
run_test "7 coders, generous burnout, FIFO" \
    "7 1500 100 50 50 2 0 fifo" "no" "2"

run_test "7 coders, generous burnout, EDF" \
    "7 1500 100 50 50 2 0 edf" "no" "2"

# large even count
run_test "8 coders, generous burnout, FIFO" \
    "8 1500 100 50 50 2 0 fifo" "no" "2"

# ── Zero debug/refactor time ──────────────────────

# debug=0 refactor=0 — coders loop back immediately after compile
# very fast re-acquisition pressure
run_test "2 coders, no debug/refactor, FIFO" \
    "2 800 100 0 0 5 0 fifo" "no" "5"

run_test "4 coders, no debug/refactor, EDF" \
    "4 800 100 0 0 5 0 edf" "no" "5"

# ── Repeated runs — non-determinism trap ──────────
# run the same case 5 times, all must agree on burnout/no-burnout

section "REPEATABILITY (same args, 5 runs must agree)"

repeat_test()
{
    local desc="$1"
    local args="$2"
    local expect="$3"
    local runs=5
    local failures=0

    echo -e "\n${YELLOW}▶ $desc (x$runs)${NC}"
    echo -e "  ${BLUE}$BIN $args${NC}"

    for i in $(seq 1 $runs); do
        out=$(timeout $TIMEOUT $BIN $args 2>&1)
        burned=$(echo "$out" | grep -c "burned out")
        if [ "$expect" = "yes" ] && [ "$burned" -lt 1 ]; then
            ((failures++))
        elif [ "$expect" = "no" ] && [ "$burned" -gt 0 ]; then
            ((failures++))
        fi
    done

    ((TOTAL++))
    if [ "$failures" -eq 0 ]; then
        echo -e "    ${GREEN}[PASS]${NC} all $runs runs agree (burnout=$expect)"
        ((PASS++))
    else
        echo -e "    ${RED}[FAIL]${NC} $failures/$runs runs disagreed (expected burnout=$expect)"
        ((FAIL++))
    fi
}

repeat_test "2 coders generous, no burnout, FIFO" \
    "2 800 100 50 50 2 0 fifo" "no"

repeat_test "4 coders generous, no burnout, EDF" \
    "4 800 100 50 50 2 0 edf" "no"

repeat_test "1 coder, always burns out, FIFO" \
    "1 400 200 100 100 2 0 fifo" "yes"

repeat_test "2 coders, burnout < compile, always burns out, FIFO" \
    "2 50 200 100 100 2 0 fifo" "yes"

repeat_test "4 coders, 5 compiles, no burnout, FIFO" \
    "4 1500 100 50 50 5 0 fifo" "no"


# ─────────────────────────────────────────────────
# SUMMARY
# ─────────────────────────────────────────────────
echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "  Results: ${GREEN}$PASS passed${NC} / ${RED}$FAIL failed${NC} / $TOTAL total"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
[ $FAIL -eq 0 ] && echo -e "  ${GREEN}ALL TESTS PASSED ✓${NC}" || echo -e "  ${RED}$FAIL TEST(S) FAILED ✗${NC}"
echo ""