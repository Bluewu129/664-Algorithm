#!/usr/bin/env bash
# =============================================================================
# bench.sh - Compare Held-Karp vs Greedy SCS on randomly generated inputs.
#
# Generates test cases with n fragments each, runs both solvers,
# and prints a side-by-side comparison of time and result length.
# If n > 22, only Greedy is run (Held-Karp is too slow beyond that point).
#
# Usage:
#   ./bench.sh <n> [options]
#
# Options:
#   -cases <k>      number of test cases to run (default: 5)
#   -len <l>        fragment length; in random mode fixes length instead of
#                   using 1-30 random (default: 6 normal, 1-30 random)
#   -random         use full printable ASCII charset
#   -random <k>     use k randomly chosen characters
#
# Examples:
#   ./bench.sh 8                        5 cases, DNA length 6
#   ./bench.sh 8 -cases 10              10 cases, DNA length 6
#   ./bench.sh 8 -len 12                5 cases, DNA length 12
#   ./bench.sh 8 -cases 10 -len 12      10 cases, DNA length 12
#   ./bench.sh 8 -random                5 cases, full ASCII, length 1-30
#   ./bench.sh 8 -random 5              5 cases, 5 chars, length 1-30
#   ./bench.sh 8 -random -cases 3       3 cases, full ASCII, length 1-30
#   ./bench.sh 8 -random 5 -cases 3 -len 15   3 cases, 5 chars, fixed length 15
#
# Requirements:
#   - ./held-karp and ./greedy_scs compiled in the current directory
#   - python3 available in PATH
# =============================================================================

set -euo pipefail

HK_LIMIT=22    # max n for which Held-Karp is run
FRAG_MIN=1     # min fragment length in random mode (no -len)
FRAG_MAX=30    # max fragment length in random mode (no -len)

# ── Argument validation ───────────────────────────────────────────────────────

if [[ $# -lt 1 ]]; then
    echo "Usage: ./bench.sh <n> [-cases k] [-len l] [-random [k]]"
    exit 1
fi

N="$1"
shift

if ! [[ "$N" =~ ^[0-9]+$ ]] || [[ "$N" -lt 1 ]]; then
    echo "Error: n must be a positive integer"
    exit 1
fi

# Defaults
N_CASES=5
FRAG_LEN=0       # 0 = not set by user
RANDOM_MODE=0
CHARSET_K=0      # 0 = full printable ASCII

# Parse remaining options in any order
while [[ $# -gt 0 ]]; do
    case "$1" in
        -cases)
            if [[ $# -lt 2 ]] || ! [[ "$2" =~ ^[0-9]+$ ]] || [[ "$2" -lt 1 ]]; then
                echo "Error: -cases requires a positive integer"
                exit 1
            fi
            N_CASES="$2"
            shift 2
            ;;
        -len)
            if [[ $# -lt 2 ]] || ! [[ "$2" =~ ^[0-9]+$ ]] || [[ "$2" -lt 1 ]]; then
                echo "Error: -len requires a positive integer"
                exit 1
            fi
            FRAG_LEN="$2"
            shift 2
            ;;
        -random)
            RANDOM_MODE=1
            # Optional: next arg is k (positive integer)
            if [[ $# -ge 2 ]] && [[ "$2" =~ ^[0-9]+$ ]] && [[ "$2" -ge 1 ]]; then
                CHARSET_K="$2"
                shift 2
            else
                shift 1
            fi
            ;;
        *)
            echo "Error: unrecognised option '$1'"
            echo "Usage: ./bench.sh <n> [-cases k] [-len l] [-random [k]]"
            exit 1
            ;;
    esac
done

# Apply -len default if not set
if [[ "$FRAG_LEN" -eq 0 ]]; then
    FRAG_LEN=6   # default for normal mode; random mode uses FRAG_MIN/FRAG_MAX
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

# Normal mode: fixed length, ATGC only, guaranteed no substring containment
gen_fragments() {
    local n="$1" frag_len="$2" seed="$3" outfile="$4"
    python3 -c "
import random, time
random.seed($seed + int(time.time() * 1000))
frags = []
attempts = 0
while len(frags) < $n:
    attempts += 1
    if attempts > 100000:
        # fallback: just emit what we have to avoid infinite loop
        break
    f = ''.join(random.choice('ATGC') for _ in range($frag_len))
    if not any(f in g or g in f for g in frags):
        frags.append(f)
for f in frags:
    print(f)
" > "$outfile"
}

# Random mode: fixed or variable length, custom charset
# If frag_fixed > 0, use that fixed length; otherwise use frag_min..frag_max
gen_fragments_random() {
    local n="$1" frag_min="$2" frag_max="$3" frag_fixed="$4" seed="$5" k="$6" outfile="$7"
    python3 -c "
import random, string
random.seed($seed)

if $k == 0:
    pool = string.printable.strip()
else:
    full_pool = string.printable.strip()
    pool = ''.join(random.sample(full_pool, min($k, len(full_pool))))

for _ in range($n):
    if $frag_fixed > 0:
        length = $frag_fixed
    else:
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
        CHARSET_DESC=$( [[ "$CHARSET_K" -eq 0 ]] && echo "full ASCII" || echo "k=$CHARSET_K chars" )
        LEN_DESC=$( [[ "$FRAG_LEN" -ne 6 ]] && echo "len=$FRAG_LEN" || echo "len=1-$FRAG_MAX" )
        echo "  SCS Benchmark  (n=$N, cases=$N_CASES)  —  Greedy only  [random: $CHARSET_DESC, $LEN_DESC]"
    else
        echo "  SCS Benchmark  (n=$N, cases=$N_CASES)  —  Greedy only  (n > $HK_LIMIT, Held-Karp skipped)  [len=$FRAG_LEN]"
    fi
else
    if [[ "$RANDOM_MODE" -eq 1 ]]; then
        CHARSET_DESC=$( [[ "$CHARSET_K" -eq 0 ]] && echo "full ASCII" || echo "k=$CHARSET_K chars" )
        LEN_DESC=$( [[ "$FRAG_LEN" -ne 6 ]] && echo "len=$FRAG_LEN" || echo "len=1-$FRAG_MAX" )
        echo "  SCS Benchmark  (n=$N, cases=$N_CASES)  —  Held-Karp vs Greedy  [random: $CHARSET_DESC, $LEN_DESC]"
    else
        echo "  SCS Benchmark  (n=$N, cases=$N_CASES)  —  Held-Karp vs Greedy  [len=$FRAG_LEN]"
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
TOTAL_HK_LEN=0; TOTAL_GR_LEN=0; TOTAL_EXCESS=0

for case_idx in $(seq 1 "$N_CASES"); do
    input_file="${TMP}/case_${case_idx}.txt"

    if [[ "$RANDOM_MODE" -eq 1 ]]; then
        gen_fragments_random "$N" "$FRAG_MIN" "$FRAG_MAX" "$FRAG_LEN" "$case_idx" "$CHARSET_K" "$input_file"
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

        # Accumulate length stats
        (( TOTAL_HK_LEN += hk_len )) || true
        (( TOTAL_GR_LEN += gr_len )) || true
        (( TOTAL_EXCESS  += gr_len - hk_len )) || true

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
    printf "  Greedy optimal rate          : %d / %d cases (%.1f%%)\n" \
        "$MATCH_COUNT" "$PASS" "$(echo "scale=1; $MATCH_COUNT * 100 / $PASS" | bc)"
    printf "  Held-Karp was faster         : %d / %d cases\n" "$HK_FASTER" "$PASS"
    printf "  Greedy was faster            : %d / %d cases\n" "$GR_FASTER" "$PASS"

    python3 -c "
total_hk = $TOTAL_HK_LEN
total_gr = $TOTAL_GR_LEN
total_excess = $TOTAL_EXCESS
n = $PASS

avg_ratio  = total_gr / total_hk if total_hk > 0 else 0
avg_excess = total_excess / n    if n > 0 else 0

print()
print(f'  Avg GR length / HK length    : {avg_ratio:.4f}  ({(avg_ratio-1)*100:+.2f}%)')
print(f'  Avg excess chars (GR - HK)   : {avg_excess:+.2f} chars')
"
fi
echo ""