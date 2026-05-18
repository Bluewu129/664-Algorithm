# SCS Benchmark

Compare **Held-Karp** (exact) vs **Greedy** (approximate) algorithms for the Shortest Common Superstring (SCS) problem on randomly generated inputs.

## Files

| File | Description |
|------|-------------|
| `held-karp.c` | Exact SCS solver using Held-Karp dynamic programming |
| `greedy_scs.c` | Approximate SCS solver using a greedy overlap strategy |
| `bench.sh` | Benchmarking script that runs both solvers and compares results |

## Requirements

- `gcc` (to compile the solvers)
- `python3` (used internally by `bench.sh` for input generation and timing)
- `bash`

## Setup

```bash
gcc -O2 -o held-karp held-karp.c
gcc -O2 -o greedy_scs greedy_scs.c
```

## Usage

```bash
./bench.sh <n> [options]
```

`n` is the number of fragments per test case.

> **Note:** Held-Karp is only run when `n ≤ 22`. For larger inputs, only Greedy is run.

### Options

| Option | Description | Default |
|--------|-------------|---------|
| `-cases <k>` | Number of test cases to run | `5` |
| `-len <l>` | Fragment length | `6` (normal mode), `1–30` (random mode) |
| `-random` | Use full printable ASCII charset with variable-length fragments | off |
| `-random <k>` | Use `k` randomly chosen characters | full ASCII |

### Examples

```bash
# 5 cases, n=8 fragments, DNA alphabet (ATGC), length 6
./bench.sh 8

# 10 cases, length 12
./bench.sh 8 -cases 10 -len 12

# 5 cases, full ASCII charset, variable fragment length (1–30)
./bench.sh 8 -random

# 3 cases, 5-character alphabet, fixed length 15
./bench.sh 8 -random 5 -cases 3 -len 15

# n=30, Greedy only (Held-Karp skipped for n > 22)
./bench.sh 30 -cases 10
```

## Output

The script prints a side-by-side table of results:

```
==============================================================
  SCS Benchmark  (n=8, cases=5)  —  Held-Karp vs Greedy  [len=6]
==============================================================

case     HK time  HK result                   GR time  GR result                 verdict
─────────────────────────────────────────────────────────────────────────────────────────
1         1823µs  ATGCATGC...                   312µs  ATGCATGC...               same length
2         2041µs  GCTAGCTA...                   289µs  GCTAGCTA...               HK shorter by 2
...
─────────────────────────────────────────────────────────────────────────────────────────
  Done: 5 passed   0 failed

  Greedy optimal rate          : 3 / 5 cases (60.0%)
  Held-Karp was faster         : 0 / 5 cases
  Greedy was faster            : 5 / 5 cases

  Avg GR length / HK length    : 1.0123  (+1.23%)
  Avg excess chars (GR - HK)   : +0.40 chars
```

### Summary Statistics (when both solvers run)

- **Greedy optimal rate** — how often Greedy matches the exact Held-Karp length
- **Speed comparison** — how often each solver was faster
- **Avg GR / HK length ratio** — how much longer Greedy's output is on average
- **Avg excess chars** — average character overhead of Greedy vs optimal