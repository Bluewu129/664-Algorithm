# Preprocessing — Role C (Week 9)

Shared preprocessing layer for both submitted algorithms. Plugs in front of
member A's Greedy SSP and member B's Plain Bitmask DP solvers.

## What's in here

| File | Purpose |
|---|---|
| `preprocessing-design.md` | English spec — data structures, function signatures, pseudocode, correctness, complexity, worked example |
| `preprocessing-design_zh.md` | Chinese translation of the design document |
| `preprocess_demo.c` | C prototype: `load_fragments`, `prune_substrings`, `compute_overlaps`, plus a smoke-test driver |

## Compile & run the prototype

```bash
gcc -O2 -o preprocess_demo preprocess_demo.c
./preprocess_demo ../tests/<fixture>.txt
cat ../tests/<fixture>.txt | ./preprocess_demo -
```

Output shows the loaded fragments, the pruned set, and the overlap matrix.

## Public interface (the contract for A and B)

```c
Fragments      load_fragments(const char *file_name);
int            prune_substrings(Fragments *f);          // returns # removed
OverlapMatrix  compute_overlaps(const Fragments *f);
void           free_fragments(Fragments *f);
void           free_overlap_matrix(OverlapMatrix *o);
```

Both algorithms operate on `(pruned fragments, O)` only — no string matching
inside their main loop.

## Hand-off

Submission rule is "one self-contained `.c` per algorithm", so the
preprocessing code must be **pasted** into each algorithm file rather than
`#include`d. Week 10's Role C produces the canonical snippet for A and B to
embed.
