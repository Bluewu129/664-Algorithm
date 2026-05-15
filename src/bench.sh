#!/usr/bin/env bash
# =============================================================================
# bench.sh - Compare Held-Karp vs Greedy SCS on randomly generated inputs.
#
# Generates 5 random test cases with n fragments each, runs both solvers,
# and prints a side-by-side comparison of time and result length.
# If n > 22, only Greedy is run (Held-Karp is too slow beyond that point).
#
# Usage:
#   ./bench.sh <n>
#   ./bench.sh <n> -random          # random fragment lengths (1-30), full ASCII printable charset
#   ./bench.sh <n> -random <k>      # random fragment lengths (1-30), k randomly chosen characters
#
# Examples:
#   ./bench.sh 8              run both solvers, DNA fragments of length 6
#   ./bench.sh 25             run greedy only (n > 22), DNA fragments of length 6
#   ./bench.sh 8 -random      random lengths, full printable ASCII charset
#   ./bench.sh 8 -random 5    random lengths, 5 randomly chosen characters
#
# Requirements:
#   - ./held-karp and ./greedy_scs compiled in the current directory
#   - python3 available in PATH
# =============================================================================

set -euo pipefail

HK_LIMIT=22    # max n for which Held-Karp is run
N_CASES=5      # number of test cases per run
FRAG_LEN=6     # length of each generated fragment (normal mode)
FRAG_MIN=1     # min fragment length in random mode
FRAG_MAX=30    # max fragment length in random mode

# ── Argument validation ───────────────────────────────────────────────────────

if [[ $# -lt 1 ]]; then
    echo "Usage: ./bench.sh <n> [-random [k]]"
    echo "  n = number of fragments per test case"
    echo "  -random     = random fragment lengths + full printable ASCII charset"
    echo "  -random k   = random fragment lengths + k randomly chosen characters"
    exit 1
fi

N="$1"

if ! [[ "$N" =~ ^[0-9]+$ ]] || [[ "$N" -lt 1 ]]; then
    echo "Error: n must be a positive integer"
    exit 1
fi

# Parse -random and optional k
RANDOM_MODE=0
CHARSET_K=0   # 0 means full printable ASCII

if [[ $# -ge 2 ]]; then
    if [[ "$2" != "-random" ]]; then
        echo "Error: unrecognised option '$2'. Did you mean -random?"
        exit 1
    fi
    RANDOM_MODE=1
    if [[ $# -ge 3 ]]; then
        if ! [[ "$3" =~ ^[0-9]+$ ]] || [[ "$3" -lt 1 ]]; then
            echo "Error: k must be a positive integer"
            exit 1
        fi
        CHARSET_K="$3"
    fi
fi

# ── Check binaries ────────────────────────────────────────────────────────────

MISSING=0
if [[ ! -x "./greedy_scs" ]]; then
    echo "Error: ./greedy_scs not found. Please compile first:"
    echo "       gcc -O2 -o greedy_scs greedy_scs.c"
    MISSING=1
fi
if [[ "$N" -le "$HK_LIMIT" ]] && [[ ! -x "./held-karp" ]]; then
    echo "Error: ./held-karp not found. Please compile first:"
    echo "       gcc -O2 -o held-karp held-karp.c"
    MISSING=1
fi
[[ "$MISSING" -eq 1 ]] && exit 1

# ── Input generation ──────────────────────────────────────────────────────────

# Normal mode: fixed length, ATGC only
gen_fragments() {
    local n="$1" frag_len="$2" seed="$3" outfile="$4"
    python3 -c "
import random
random.seed($seed)
for _ in range($n):
    print(''.join(random.choice('ATGC') for _ in range($frag_len)))
" > "$outfile"
}

# Random mode: variable length (FRAG_MIN-FRAG_MAX), custom charset
gen_fragments_random() {
    local n="$1" frag_min="$2" frag_max="$3" seed="$4" k="$5" outfile="$6"
    python3 -c "
import random, string
random.seed($seed)

# Build charset
if $k == 0:
    pool = string.printable.strip()          # full printable ASCII, no whitespace
else:
    full_pool = string.printable.strip()
    pool = ''.join(random.sample(full_pool, min($k, len(full_pool))))

for _ in range($n):
    length = random.randint($frag_min, $frag_max)
    print(''.join(random.choice(pool) for _ in range(length)))
" > "$outfile"
}

# ── Timing helper ─────────────────────────────────────────────────────────────

run_timed() {
    local binary="$1" input="$2"
    python3 -c "
import sys, time, subprocess
binary, input_file = '$binary', '$input'
t0 = time.monotonic()
r = subprocess.run([binary, input_file], capture_output=True, text=True)
t1 = time.monotonic()
if r.returncode != 0:
    print('FAILED')
    sys.exit(0)
elapsed_us = int((t1 - t0) * 1e6)
scs = r.stdout.strip()
if scs:
    print(elapsed_us, scs)
else:
    print('FAILED')
"
}

# ── Temporary directory ───────────────────────────────────────────────────────

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# ── Print header ──────────────────────────────────────────────────────────────

echo ""
echo "=============================================================="
if [[ "$N" -gt "$HK_LIMIT" ]]; then
    if [[ "$RANDOM_MODE" -eq 1 ]]; then
        if [[ "$CHARSET_K" -eq 0 ]]; then
            echo "  SCS Benchmark  (n=$N)  —  Greedy only  [random mode, full ASCII]"
        else
            echo "  SCS Benchmark  (n=$N)  —  Greedy only  [random mode, k=$CHARSET_K chars]"
        fi
    else
        echo "  SCS Benchmark  (n=$N)  —  Greedy only  (n > $HK_LIMIT, Held-Karp skipped)"
    fi
else
    if [[ "$RANDOM_MODE" -eq 1 ]]; then
        if [[ "$CHARSET_K" -eq 0 ]]; then
            echo "  SCS Benchmark  (n=$N)  —  Held-Karp vs Greedy  [random mode, full ASCII]"
        else
            echo "  SCS Benchmark  (n=$N)  —  Held-Karp vs Greedy  [random mode, k=$CHARSET_K chars]"
        fi
    else
        echo "  SCS Benchmark  (n=$N)  —  Held-Karp vs Greedy"
    fi
fi
echo "=============================================================="

if [[ "$N" -le "$HK_LIMIT" ]]; then
    printf "\n%-5s  %10s  %-24s  %10s  %-24s  %s\n" \
        "case" "HK time" "HK result" "GR time" "GR result" "verdict"
else
    printf "\n%-5s  %10s  %s\n" "case" "GR time" "GR result"
fi
printf '%.0s─' {1..85}; echo ""

# ── Run test cases ────────────────────────────────────────────────────────────

PASS=0; FAIL=0
HK_FASTER=0; GR_FASTER=0; MATCH_COUNT=0

for case_idx in $(seq 1 "$N_CASES"); do
    input_file="${TMP}/case_${case_idx}.txt"

    if [[ "$RANDOM_MODE" -eq 1 ]]; then
        gen_fragments_random "$N" "$FRAG_MIN" "$FRAG_MAX" "$case_idx" "$CHARSET_K" "$input_file"
    else
        gen_fragments "$N" "$FRAG_LEN" "$case_idx" "$input_file"
    fi

    if [[ "$N" -gt "$HK_LIMIT" ]]; then
        # Greedy only
        gr_out=$(run_timed "./greedy_scs" "$input_file")
        if [[ "$gr_out" == "FAILED" ]]; then
            printf "%-5s  %10s  %s\n" "$case_idx" "—" "[greedy failed]"
            (( FAIL++ )) || true
        else
            gr_us=$(echo "$gr_out"  | awk '{print $1}')
            gr_scs=$(echo "$gr_out" | awk '{print $2}')
            printf "%-5s  %9sµs  %s\n" "$case_idx" "$gr_us" "$gr_scs"
            (( PASS++ )) || true
        fi
    else
        # Both solvers
        hk_out=$(run_timed "./held-karp"  "$input_file")
        gr_out=$(run_timed "./greedy_scs" "$input_file")

        if [[ "$hk_out" == "FAILED" ]] || [[ "$gr_out" == "FAILED" ]]; then
            printf "%-5s  %10s  %-24s  %10s  %-24s  %s\n" \
                "$case_idx" "—" "[failed]" "—" "[failed]" "—"
            (( FAIL++ )) || true
            continue
        fi

        hk_us=$(echo "$hk_out"  | awk '{print $1}')
        hk_scs=$(echo "$hk_out" | awk '{print $2}')
        gr_us=$(echo "$gr_out"  | awk '{print $1}')
        gr_scs=$(echo "$gr_out" | awk '{print $2}')

        hk_len=${#hk_scs}
        gr_len=${#gr_scs}

        # Compare result lengths
        if [[ "$hk_len" -eq "$gr_len" ]]; then
            verdict="same length"
            (( MATCH_COUNT++ )) || true
        elif [[ "$hk_len" -lt "$gr_len" ]]; then
            verdict="HK shorter by $(( gr_len - hk_len ))"
        else
            verdict="GR shorter by $(( hk_len - gr_len ))"
        fi

        # Track which solver was faster
        if [[ "$hk_us" -lt "$gr_us" ]]; then
            (( HK_FASTER++ )) || true
        else
            (( GR_FASTER++ )) || true
        fi

        printf "%-5s  %9sµs  %-24s  %9sµs  %-24s  %s\n" \
            "$case_idx" "$hk_us" "$hk_scs" "$gr_us" "$gr_scs" "$verdict"

        (( PASS++ )) || true
    fi
done

# ── Summary ───────────────────────────────────────────────────────────────────

printf '%.0s─' {1..85}; echo ""
printf "  Done: %d passed   %d failed\n" "$PASS" "$FAIL"

if [[ "$N" -le "$HK_LIMIT" ]] && [[ "$PASS" -gt 0 ]]; then
    echo ""
    printf "  Same length (greedy optimal) : %d / %d cases\n" "$MATCH_COUNT" "$PASS"
    printf "  Held-Karp was faster         : %d / %d cases\n" "$HK_FASTER"   "$PASS"
    printf "  Greedy was faster            : %d / %d cases\n" "$GR_FASTER"   "$PASS"
fi
echo ""