# IFN664 A1 — Role C handover (2026-05-10)

Group meeting talking points. Scope is strictly Role C (shared
preprocessing + complexity analysis). Algorithm cores in
`src/held-karp.c` and `src/greedy_scs.c` are **unchanged** — only the
input loader and the redundant in-file copies of preprocessing have
been replaced with the canonical `preprocessing/preprocess_canonical.c`
block.

The submission rule (`CLAUDE.md`) requires each algorithm to be a
single self-contained `.c`, so the canonical block is **pasted**
verbatim into both algorithm files. When a fix lands in
`preprocessing/preprocess_canonical.c`, mirror it to the two algorithm
files in the same commit.

Verdict after the changes:
- `scripts/run_all.sh` → `alg2_tgreedy` is **20/20 OPTIMAL** on every fixture.
- `alg1_bitmask_dp` shows 20/20 INVALID under the harness only because
  Role B's banner output (`=== Held-Karp ===`, `[Info]`, `[Result]`,
  `[Verification]`) is not parseable by `scripts/validate.sh`. Manual
  extraction of the `Shortest Common Superstring :` line confirms the
  algorithm produces a string of the correct optimal length on **all 20**
  fixtures. **Cleaning that stdout is Role B's call**, flagged below.

---

## src/held-karp.c — input + redundancy only

| File / location | Before | After | Why |
|---|---|---|---|
| `main` body (old lines 297–326) | Interactive `scanf`: prompts for `n` and each fragment over stdin. | File-arg loader: `./alg1_bitmask_dp <input>` (or `-` for stdin). Calls canonical `load_fragments`, `prune_substrings`, `compute_overlaps`, then **copies the results into the existing `fragments[][]`, `frag_len[]`, `overlap_matrix[][]`, `n` static arrays** so the rest of the file runs unchanged. | Test harness, validator, and end-to-end pipeline all need a file path on `argv[1]`. The `scanf` driver couldn't be tested non-interactively. |
| `compute_overlap`, `is_substring`, `remove_substrings`, `build_overlap_matrix` (old lines 42–114, ~73 LOC) | Inline reimplementations of preprocessing logic that I already own. | **Deleted.** Replaced by calls into the canonical block in `main`. | Exactly the "simple redundancy removal" duplication the todo flagged. Centralising in preprocessing is Role C's responsibility. |
| Forward declarations at top (old lines 22–25) | Declared the four removed functions. | Removed those four declarations only. | Bookkeeping — the functions are gone. |
| `=== Held-Karp ===` banner, `[Info]`, `[Overlap Matrix]`, `[Result]`, `[Verification]` blocks | Printed to stdout. | **Unchanged. Stdout output is identical to the original.** | Role B's stylistic/diagnostic choice. Not Role C's call to strip. |
| `held_karp()`, `reconstruct()`, `print_overlap_matrix()`, `verify_result()` | DP transitions, parent-pointer traceback, debug prints. | **Verbatim, no edits.** | These are Role B's algorithm logic. Untouched. |
| `dp[1<<MAX_N][MAX_N]`, `parent_mask[…]`, `parent_last[…]`, `MAX_N=20`, `MAX_LEN=100` | Static globals, fixed sizes. | **Unchanged.** | Inside the n ≤ 20 scope the assignment expects, this is fine. |
| Build target | `gcc -O2 -o held-karp src/held-karp.c` | `gcc -O2 -o alg1_bitmask_dp src/held-karp.c` | `scripts/run_all.sh` registers the binary as `alg1_bitmask_dp`. |

### Side effect: tie-break for identical fragments

The original `remove_substrings` marked **both** copies of identical
fragments as dominated and erased both. Now that pruning goes through
canonical `prune_substrings`, the deterministic tie-break "lower-index
copy survives" applies, and `tests/05-all_identical.txt` produces `ab`
rather than an empty string. Same code-quality benefit; no algorithm
change.

## src/greedy_scs.c — input + redundancy only

| File / location | Before | After | Why |
|---|---|---|---|
| `struct fragment_s`, `read_line`, `read_all_fragments`, `free_all_fragments`, `list_to_array` (old lines 14–104) | Linked-list-based input loader, then flatten to `char**`. | **Deleted.** Replaced by canonical `load_fragments` in `main`, then string ownership is transferred into the existing `char**` API. | Eliminates a duplicate of preprocessing's input loading. The portable fgetc-based reader survives inside the canonical block (`pp_read_line`); we still don't depend on POSIX `getline`. |
| `remove_dominated` (old lines 146–166) | Inline substring elimination. | **Deleted.** Pre-call to canonical `prune_substrings` happens in `main` before `greedy_scs` runs. | Same redundancy fix, plus the deterministic tie-break for identical fragments. |
| `greedy_scs()` body | First line was `remove_dominated(arr, &n);`. | **Removed that one line.** Everything after it (the merge loop, `calculate_overlap`, `merge`) is byte-for-byte unchanged. | Strictly the "remove redundant logic with my preprocessing" the user asked for. The merge loop itself is Role A's algorithm — left intact. |
| `calculate_overlap`, `merge` | Inline. | **Verbatim, no edits.** | These run inside the merge loop on the live (mutated) array; the precomputed n×n matrix from preprocessing is invalid after the first merge, so they're not redundant. |
| stdout | One line, no banners. | **Unchanged.** Already harness-compatible. | — |
| Build target | `gcc -O2 -o greedy_scs greedy_scs.c` | `gcc -O2 -o alg2_tgreedy src/greedy_scs.c` | Harness expects `./alg2_tgreedy`. |

## Things I deliberately did not touch

- **Held-Karp banner output / `[Info]` / `[Result]` / `[Verification]`**: that's
  Role B's choice. If the team wants harness compatibility, Role B should
  route those prints to stderr (or remove them). Easy follow-up; not Role C's call.
- **MAX_N=20 / MAX_LEN=100 in held-karp.c**: kept. The assignment scope is
  n ≤ ~20 anyway.
- **Static dp / parent_mask / parent_last arrays**: kept. The Role C
  preprocessing layer doesn't need to touch the DP state machine.
- **POSIX `getline()`**: still not used. `CLAUDE.md` warns the marker's
  toolchain may not have it. The canonical block ships fgetc-based
  `pp_read_line()` instead.
- **No shared header.** Submission rule = one self-contained `.c` per
  algorithm. Canonical block exists only as a paste source.

## End-to-end test results

```bash
gcc -O2 -o alg1_bitmask_dp                src/held-karp.c
gcc -O2 -o alg2_tgreedy                   src/greedy_scs.c
gcc -O2 -std=c2x -o brute-force_text_reconstruction  src/brute-force_text_reconstruction.c
./scripts/run_all.sh
```

| Algorithm | Harness verdict | Manual verdict |
|---|---|---|
| `ref_bf` (oracle) | 11 OPTIMAL / 9 TIMEOUT (expected per `CLAUDE.md`) | — |
| `alg1_bitmask_dp` | **20/20 INVALID** (harness can't parse banners) | **20/20 OK** — `grep "Shortest Common Superstring :"` extracts a string of the correct optimal length on every fixture, including `tests/05-all_identical.txt` (`ab`) |
| `alg2_tgreedy` | **20/20 OPTIMAL** | — |

So the algorithms themselves are correct; only Held-Karp's stdout format
prevents the harness from validating it. Cleanup is Role B's.

## Open question for the group

`alg2_tgreedy` hit OPTIMAL on every fixture, not just CORRECT. That's
luckier than the 4-approximation guarantee. Worth deciding before the
report whether to (a) add an adversarial fixture that forces greedy
below optimal so we can show the gap empirically, or (b) note the
empirical optimality and discuss why our fixtures don't exercise the
worst case.

## Suggested follow-ups for Role A and Role B

- **Role B (`src/held-karp.c`)**: route `=== Held-Karp ===` / `[Info]` /
  `[Overlap Matrix]` / `[Result]` / `[Verification]` prints to `stderr`,
  keep only the SCS string + `\n` on stdout. That single change moves
  alg1 from 20 INVALID → 20 OPTIMAL in the harness without touching DP
  logic.
- **Role A (`src/greedy_scs.c`)**: nothing pending from Role C. The
  greedy core is unmodified and already harness-compatible.
