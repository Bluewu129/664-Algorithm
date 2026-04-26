#!/usr/bin/env bash
# run_all.sh — Run every registered algorithm against every fixture; emit a
#              pass/fail matrix using validate.sh + .expected sidecar files.
#
# Portable: works on macOS's default bash 3.2 (no `timeout` / `gtimeout`
# required — a pure-shell watchdog provides the kill-after-N-seconds).

set -u

HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
TESTS="$ROOT/tests"
VALIDATE="$HERE/validate.sh"

# --- Algorithm registry (parallel arrays, index-aligned) ------------------
ALGO_NAMES=(ref_bf alg1_dp alg2_greedy)
ALGO_PATHS=(
    "$ROOT/brute-force_text_reconstruction"
    "$ROOT/alg1_bitmask_dp"
    "$ROOT/alg2_tgreedy"
)
ALGO_TIMEOUTS=(30 30 30)

# --- Shell-level timeout (runs command, writes stdout to $2, SIGKILLs after $1 s)
# Returns the child's real exit code, or 137 if the watchdog had to kill it.
run_with_timeout() {
    local secs="$1"; shift
    local outfile="$1"; shift
    "$@" > "$outfile" 2>/dev/null &
    local pid=$!
    (
        sleep "$secs"
        kill -KILL "$pid" 2>/dev/null
    ) 2>/dev/null &
    local watchdog=$!
    wait "$pid" 2>/dev/null
    local rc=$?
    # Child finished on its own → tear down the idle watchdog silently.
    kill -KILL "$watchdog" 2>/dev/null
    wait "$watchdog" 2>/dev/null
    return $rc
}

# --- Discover fixtures (a fixture = .txt with matching .expected) ---------
FIXTURES=()
for tf in "$TESTS"/??-*.txt; do
    base=$(basename "$tf" .txt)
    ef="$TESTS/${base}.expected"
    if [ -f "$ef" ]; then
        FIXTURES+=("$tf|$ef")
    fi
done

# --- Header ----------------------------------------------------------------
printf "%-28s" "Fixture"
for a in "${ALGO_NAMES[@]}"; do printf " | %-11s" "$a"; done
printf "\n"
printf -- "-%.0s" $(seq 1 $((28 + 14 * ${#ALGO_NAMES[@]})))
printf "\n"

tot_opt=0; tot_cor=0; tot_inv=0; tot_to=0; tot_skip=0
TMPOUT="$(mktemp)"
trap 'rm -f "$TMPOUT"' EXIT

for entry in "${FIXTURES[@]}"; do
    tf="${entry%|*}"; ef="${entry#*|}"
    label=$(basename "$tf" .txt)
    expected_len=$(awk '/^LEN /{print $2; exit}' "$ef")
    printf "%-28s" "$label"

    i=0
    while [ $i -lt ${#ALGO_NAMES[@]} ]; do
        bin="${ALGO_PATHS[$i]}"
        to="${ALGO_TIMEOUTS[$i]}"

        if [ ! -x "$bin" ]; then
            printf " | %-11s" "SKIP"
            tot_skip=$((tot_skip + 1))
            i=$((i + 1)); continue
        fi

        run_with_timeout "$to" "$TMPOUT" "$bin" "$tf"; rc=$?
        if [ "$rc" -eq 124 ] || [ "$rc" -eq 137 ] || [ "$rc" -eq 143 ]; then
            printf " | %-11s" "TIMEOUT"
            tot_to=$((tot_to + 1))
            i=$((i + 1)); continue
        fi

        # Legacy BF concatenates multiple solutions with no separator;
        # split by expected length so validator sees one solution per line.
        out_clean=$(tr -d '\n\r ' < "$TMPOUT")
        if [ -n "$expected_len" ] && [ "$expected_len" -gt 0 ] && [ -n "$out_clean" ]; then
            cands=$(printf "%s" "$out_clean" | fold -w "$expected_len")
        else
            cands=$(cat "$TMPOUT")
        fi

        status_line=$(printf "%s\n" "$cands" | "$VALIDATE" --file "$tf" --stdin --expected "$ef" --quiet 2>/dev/null | head -1)
        st=$(printf "%s" "$status_line" | awk -F'[ =]' '{print $2}')
        printf " | %-11s" "${st:-?}"
        case "$st" in
            OPTIMAL) tot_opt=$((tot_opt + 1)) ;;
            CORRECT) tot_cor=$((tot_cor + 1)) ;;
            SHORTER) tot_cor=$((tot_cor + 1)) ;;
            VALID)   tot_cor=$((tot_cor + 1)) ;;
            INVALID) tot_inv=$((tot_inv + 1)) ;;
        esac

        i=$((i + 1))
    done
    printf "\n"
done

echo
echo "Summary: $tot_opt OPTIMAL, $tot_cor CORRECT, $tot_inv INVALID, $tot_to TIMEOUT, $tot_skip SKIP"
[ "$tot_inv" -eq 0 ]
