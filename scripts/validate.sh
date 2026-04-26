#!/usr/bin/env bash
# validate.sh — Check whether candidate reconstructions cover all fragments,
#               and optionally compare their length against a known optimum.
#
# V1 mode (backward compatible):
#   ./validate.sh <input_file> <candidate_string>
#
# V2 mode:
#   ./validate.sh --file INPUT_FILE --candidates CAND_FILE [--expected EXP_FILE] [flags]
#   ./validate.sh --file INPUT_FILE --stdin             [--expected EXP_FILE] [flags]
#
# Flags:
#   --strict     require every candidate to have identical length
#   --quiet      machine-readable STATUS line only, no prose
#   --verbose    list fragment positions for each candidate
#
# Output (machine-readable first line):
#   STATUS=OPTIMAL    length=L count=N
#   STATUS=CORRECT    length=L count=N      # all valid, lengths > expected LEN
#   STATUS=INVALID    missing=M              # at least one candidate missing fragments
#   STATUS=NONUNIFORM lengths=[L1,L2,...]    # --strict caught mixed lengths
#
# Exit codes:
#   0 = OPTIMAL (or VALID when no --expected given)
#   1 = CORRECT but suboptimal
#   2 = INVALID (missing fragment in some candidate)
#   3 = NONUNIFORM (mixed lengths under --strict)
#   4 = usage / IO error

set -u

STRICT=0
QUIET=0
VERBOSE=0
INPUT=""
CAND_FILE=""
EXPECTED_FILE=""
USE_STDIN=0

usage() {
    sed -n '2,30p' "$0" >&2
    exit 4
}

# ---------- V1 backward-compat shim ----------
# If called with exactly 2 positional args and no leading --, treat as V1.
if [ "$#" -eq 2 ] && [ "${1:0:2}" != "--" ] && [ "${2:0:2}" != "--" ]; then
    input_file="$1"
    candidate="$2"
    if [ ! -f "$input_file" ]; then
        echo "Error: input file not found: $input_file" >&2
        exit 4
    fi
    missing=0
    while IFS= read -r fragment || [ -n "$fragment" ]; do
        [ -z "$fragment" ] && continue
        case "$candidate" in
            *"$fragment"*) ;;
            *)
                echo "MISSING: $fragment"
                missing=$((missing + 1))
                ;;
        esac
    done < "$input_file"
    echo "Candidate length: ${#candidate}"
    if [ "$missing" -eq 0 ]; then
        echo "VALID: all fragments present."
        exit 0
    else
        echo "INVALID: $missing fragment(s) missing."
        exit 2
    fi
fi

# ---------- V2 flag parsing ----------
while [ "$#" -gt 0 ]; do
    case "$1" in
        --file)        INPUT="$2"; shift 2 ;;
        --candidates)  CAND_FILE="$2"; shift 2 ;;
        --stdin)       USE_STDIN=1; shift ;;
        --expected)    EXPECTED_FILE="$2"; shift 2 ;;
        --strict)      STRICT=1; shift ;;
        --quiet)       QUIET=1; shift ;;
        --verbose)     VERBOSE=1; shift ;;
        -h|--help)     usage ;;
        *)             echo "Unknown arg: $1" >&2; usage ;;
    esac
done

[ -z "$INPUT" ] && { echo "--file is required" >&2; exit 4; }
[ ! -f "$INPUT" ] && { echo "Input file not found: $INPUT" >&2; exit 4; }
if [ "$USE_STDIN" -eq 0 ] && [ -z "$CAND_FILE" ]; then
    echo "Need --candidates FILE or --stdin" >&2; exit 4
fi
if [ "$USE_STDIN" -eq 1 ] && [ -n "$CAND_FILE" ]; then
    echo "Use either --candidates or --stdin, not both" >&2; exit 4
fi

# ---------- Load fragments into an array ----------
FRAGMENTS=()
while IFS= read -r line || [ -n "$line" ]; do
    [ -z "$line" ] && continue
    FRAGMENTS+=("$line")
done < "$INPUT"

# ---------- Parse --expected sidecar (LEN <n> + optional known optimal strings) ----------
EXPECTED_LEN=""
if [ -n "$EXPECTED_FILE" ]; then
    if [ ! -f "$EXPECTED_FILE" ]; then
        echo "Expected file not found: $EXPECTED_FILE" >&2; exit 4
    fi
    while IFS= read -r line || [ -n "$line" ]; do
        # Skip blanks and comments
        [ -z "$line" ] && continue
        case "$line" in \#*) continue ;; esac
        case "$line" in
            LEN\ *) EXPECTED_LEN="${line#LEN }" ;;
        esac
    done < "$EXPECTED_FILE"
fi

# ---------- Read candidates (one per line) ----------
CANDIDATES=()
if [ "$USE_STDIN" -eq 1 ]; then
    while IFS= read -r line || [ -n "$line" ]; do
        [ -z "$line" ] && continue
        case "$line" in \#*) continue ;; esac
        CANDIDATES+=("$line")
    done
else
    while IFS= read -r line || [ -n "$line" ]; do
        [ -z "$line" ] && continue
        case "$line" in \#*) continue ;; esac
        CANDIDATES+=("$line")
    done < "$CAND_FILE"
fi

if [ "${#CANDIDATES[@]}" -eq 0 ]; then
    echo "STATUS=INVALID missing=all (no candidates)"
    exit 2
fi

# ---------- Validate each candidate ----------
total_missing=0
invalid_count=0
lengths=()
for cand in "${CANDIDATES[@]}"; do
    lengths+=("${#cand}")
    miss=0
    for frag in "${FRAGMENTS[@]}"; do
        case "$cand" in
            *"$frag"*)
                [ "$VERBOSE" -eq 1 ] && echo "  [${cand:0:20}...] has '$frag'"
                ;;
            *)
                miss=$((miss + 1))
                [ "$QUIET" -eq 0 ] && echo "  MISSING in \"$cand\": $frag"
                ;;
        esac
    done
    if [ "$miss" -gt 0 ]; then
        invalid_count=$((invalid_count + 1))
        total_missing=$((total_missing + miss))
    fi
done

# ---------- Uniform-length check (used by --strict and NONUNIFORM report) ----------
uniq_lengths=$(printf "%s\n" "${lengths[@]}" | sort -u | paste -sd, -)
distinct_count=$(printf "%s\n" "${lengths[@]}" | sort -u | wc -l | tr -d ' ')

# ---------- Decide STATUS ----------
if [ "$invalid_count" -gt 0 ]; then
    STATUS="INVALID"
    echo "STATUS=INVALID missing=$total_missing invalid_candidates=$invalid_count"
    exit 2
fi

if [ "$STRICT" -eq 1 ] && [ "$distinct_count" -gt 1 ]; then
    echo "STATUS=NONUNIFORM lengths=[$uniq_lengths]"
    exit 3
fi

# All candidates valid. Determine OPTIMAL vs CORRECT via --expected.
some_len="${lengths[0]}"
if [ -n "$EXPECTED_LEN" ]; then
    min_len="$some_len"
    for l in "${lengths[@]}"; do [ "$l" -lt "$min_len" ] && min_len="$l"; done
    if [ "$min_len" -eq "$EXPECTED_LEN" ]; then
        echo "STATUS=OPTIMAL length=$EXPECTED_LEN count=${#CANDIDATES[@]}"
        exit 0
    elif [ "$min_len" -lt "$EXPECTED_LEN" ]; then
        echo "STATUS=SHORTER length=$min_len expected=$EXPECTED_LEN count=${#CANDIDATES[@]}"
        echo "  NOTE: candidate is shorter than the recorded optimum — .expected may be wrong."
        exit 1
    else
        echo "STATUS=CORRECT length=$min_len expected=$EXPECTED_LEN count=${#CANDIDATES[@]}"
        exit 1
    fi
else
    echo "STATUS=VALID length=$some_len count=${#CANDIDATES[@]}"
    exit 0
fi
