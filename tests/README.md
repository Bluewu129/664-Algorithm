# Test Suite — Minimum-Size Text Reconstruction

**Last updated:** 2026-04-26

Twenty input fixtures covering the full difficulty spectrum for the
**Minimum-Size Text Reconstruction from Overlapping Fragments** problem.
Every fixture ships with a matching `<n>-*.expected` sidecar that records
the known optimal length and at least one known optimal reconstruction.

The harness lives in `../scripts/run_all.sh`; the validator lives in
`../scripts/validate.sh`. A legacy Python pair (`run_tests.py` /
`validate.py`) is kept alongside as a fallback.

---

## Files in this directory

| File | Role |
|------|------|
| `01-*.txt` … `20-*.txt` | 20 input fixtures (one fragment per line) |
| `01-*.expected` … `20-*.expected` | Expected-answer sidecars (LEN + known optima) |
| `README.md` | This document |

---

## Sidecar format (`*.expected`)

```
# Any number of comment lines (start with #)
LEN <n>               ← the known optimal length
<optimal_string_1>    ← one known optimal reconstruction per remaining line
<optimal_string_2>
...
```

Blank lines and comment lines are ignored.

---

## Fixture catalog

| # | File | Category | Optimum l | Covers |
|---|------|----------|-----------|--------|
| 1 | `01-trivial_single` | Trivial | 2 | single fragment |
| 2 | `02-disjoint_chars` | Trivial | 2 | 2 disjoint 1-char fragments, 2 optima |
| 3 | `03-chain_overlap` | Small | 4 | linear chain of 2-overlaps |
| 4 | `04-example_from_spec` | Small | 4 | canonical spec example |
| 5 | `05-all_identical` | Edge | 2 | duplicates collapse |
| 6 | `06-substring_pruning` | Edge | 4 | substring pruning |
| 7 | `07-no_overlap` | Edge | 4 | zero overlap, 2 orderings |
| 8 | `08-nested_fragments` | Edge | 4 | heavy nested substrings |
| 9 | `09-medium_mixed` | Medium | 9 | 8 fragments from "algorithm" |
| 10 | `10-medium_binary` | Medium | 5 | 8 binary fragments from "01101" |
| 11 | `11-large_n` | Large | 11 | 15 fragments from "performance" |
| 12 | `12-wide_alphabet` | Wide Σ | 13 | 12 fragments, alphabet size 13 |
| 13 | `13-cross_overlapping` | Cyclic | 8 | 5 cyclic rotations of "abcde" |
| 14 | `14-many_optima` | Many optima | 3 | 6 optimal strings, tests `emit all` |
| 15 | `15-short_repeats` | Repeats | 4 | short mutually-overlapping fragments |
| 16 | `16-heavy_pruning` | Pruning | 30 | 12 fragments, 8 substrings of 4 parents |
| 17 | `17-binary_stress` | Binary | 9 | 18 binary fragments |
| 18 | `18-no_overlap_large` | Disjoint | 16 | 8 disjoint length-2 fragments, 40320 optima |
| 19 | `19-single_char` | Trivial | 1 | smallest possible instance, single 1-char fragment |
| 20 | `20-decreasing_suffix` | Pruning | 7 | descending-length suffix chain (4 fragments are substrings) |

---

## Validator — `../scripts/validate.sh`

### Backward-compatible V1 mode

```bash
./scripts/validate.sh <input_file> <candidate_string>
# exits 0 (valid) / 2 (invalid)
```

### V2 mode

```bash
./scripts/validate.sh --file INPUT --candidates CAND_FILE [--expected EXP_FILE] [flags]
./scripts/validate.sh --file INPUT --stdin              [--expected EXP_FILE] [flags]
```

Flags:

| Flag | Meaning |
|------|---------|
| `--strict` | require every candidate to have identical length |
| `--quiet` | print only the machine-readable `STATUS=…` line |
| `--verbose` | list fragment-coverage for each candidate |

### Output (first line is machine-readable)

| First-line status | Meaning | Exit |
|-------------------|---------|-----:|
| `STATUS=OPTIMAL length=L count=N` | All candidates valid AND min length == expected LEN | 0 |
| `STATUS=VALID length=L count=N` | All candidates valid; no `--expected` file supplied | 0 |
| `STATUS=CORRECT length=L expected=E count=N` | All candidates valid; min length > E (suboptimal) | 1 |
| `STATUS=SHORTER length=L expected=E count=N` | Candidate shorter than recorded LEN → `.expected` may be wrong | 1 |
| `STATUS=INVALID missing=M invalid_candidates=K` | At least one candidate has missing fragments | 2 |
| `STATUS=NONUNIFORM lengths=[L1,L2,…]` | `--strict` set, candidates had differing lengths | 3 |

Exit code 4 = usage or IO error.

### Example uses

```bash
# Validate a single hand-typed candidate against fixture 4:
./scripts/validate.sh tests/04-example_from_spec.txt "abac"

# Feed an algorithm's full stdout to the validator (multiple solutions):
./alg1_bitmask_dp tests/14-many_optima.txt \
    | ./scripts/validate.sh --file tests/14-many_optima.txt --stdin \
                            --expected tests/14-many_optima.expected

# Expect: STATUS=OPTIMAL length=3 count=6
```

---

## Harness — `../scripts/run_all.sh`

Runs every registered algorithm against every fixture, captures
stdout, pipes through `validate.sh` with each fixture's `.expected`,
and prints a results matrix.

### Algorithm registry (top of the script)

```bash
ALGO_NAMES=(ref_bf alg1_dp alg2_greedy)
ALGO_PATHS=(
    "$ROOT/brute-force_text_reconstruction"   # provided demo — oracle, NOT submitted
    "$ROOT/alg1_bitmask_dp"                   # Alg 1
    "$ROOT/alg2_tgreedy"                      # Alg 2
)
ALGO_TIMEOUTS=(30 30 30)                      # seconds
```

`ref_bf` is the instructor's brute-force demo, used as a ground-truth
oracle for small fixtures. It is **not** part of our submission; its
column exists for differential testing and speed comparison.

Binaries that do not exist show `SKIP` — safe to run before alternative
algorithms have been built.

### Output shape

```
Fixture                      | ref_bf      | alg1_dp     | alg2_greedy
----------------------------------------------------------------------
01-trivial_single            | OPTIMAL     | SKIP        | SKIP
04-example_from_spec         | OPTIMAL     | SKIP        | SKIP
09-medium_mixed              | TIMEOUT     | SKIP        | SKIP
...
Summary: 8 OPTIMAL, 0 CORRECT, 0 INVALID, 12 TIMEOUT, 40 SKIP
```

The script exits non-zero iff any cell is `INVALID`.

### Interpreting cells

| Cell | Meaning |
|------|---------|
| `OPTIMAL` | algorithm found a correct answer at the expected optimum length ✓ |
| `CORRECT` | algorithm answer is a valid reconstruction but longer than optimum |
| `SHORTER` | algorithm answer is SHORTER than `.expected` LEN → investigate (sidecar likely wrong) |
| `INVALID` | algorithm output is missing at least one fragment → bug |
| `TIMEOUT` | algorithm exceeded its configured timeout — expected for `ref_bf` on medium/large fixtures |
| `SKIP` | algorithm binary not found — normal until alternative algorithms are built |

---

## Typical workflow

```bash
# Compile the brute-force oracle (-std=c2x because the source uses C23 bool):
gcc -O2 -std=c2x -o brute-force_text_reconstruction src/brute-force_text_reconstruction.c

# Run the matrix:
./scripts/run_all.sh
# expect: ref_bf OPTIMAL on fixtures 1-8 + 19-20, TIMEOUT on 9-18, all SKIP elsewhere

# After alg1_bitmask_dp.c is added:
gcc -O2 -o alg1_bitmask_dp alg1_bitmask_dp.c
./scripts/run_all.sh
# expect: alg1_dp column now shows OPTIMAL across all fixtures it can finish
```

---

## Hand-off notes

1. **Output contract for additional algorithms:** the validator and harness both
   assume **one solution per line** on stdout. Stick to this to avoid needing
   the legacy concatenation-splitter code path.

2. **Sidecars with `LEN ?`:** none at the moment — all 20 fixtures have
   hand-derived optima. If future fixtures are added whose optimum is
   unknown, set `LEN ?` and the harness will degrade gracefully to VALID
   status rather than OPTIMAL.

3. **Timeouts are per-algorithm, not per-fixture.** Tweak
   `ALGO_TIMEOUTS` at the top of `run_all.sh` if any algorithm
   needs a longer budget on realistic inputs.

4. **Differential testing:** keep the `ref_bf` column. If a DP algorithm
   disagrees with the demo on any of the small fixtures where both finish,
   one of them is wrong — investigate.

5. **Legacy Python scripts** (`scripts/run_tests.py`, `scripts/validate.py`)
   are kept as a single-algorithm fallback. They predate the V2 sidecar
   format and only report PASS/FAIL — prefer the bash pair for new work.
