# Greedy Shortest Common Superstring (SCS)

**Goal:** find the shortest string that contains every input fragment as a substring.
The greedy heuristic repeatedly merges the pair of fragments with the greatest overlap until one string remains.

| Property | Value |
|----------|-------|
| Time complexity | O(n³ × k), n = fragment count, k = max fragment length |
| Optimality | **Not guaranteed** — greedy can miss the global optimum |

---

## RemoveDominated(fragments)

A fragment is **dominated** if it already appears inside another fragment.
Keeping it would waste a merge step without shortening the result.

```text
toRemove = empty set

FOR each A in fragments DO
    FOR each B in fragments DO

        IF A is not B AND A is a substring of B THEN
            ADD A to toRemove
            BREAK

REMOVE all strings in toRemove from fragments
```

> Example: `["abc", "b", "abcd"]` → remove `"abc"` and `"b"` because both are substrings of `"abcd"`

---

## Helper: CalculateOverlap(A, B)

Returns the length of the longest suffix of `A` that equals a prefix of `B`.

```text
maxK = min(length(A), length(B))

FOR k = maxK DOWN TO 1 DO
    IF suffix of A with length k == prefix of B with length k THEN
        RETURN k

RETURN 0
```

> Example: `CalculateOverlap("abcd", "cdef")` = 2 — the shared `"cd"`

---

## Helper: Merge(A, B, overlap)

Concatenates `A` and `B` using their known overlap length.

```text
RETURN A + B[overlap:]
```

> Example: `Merge("abcd", "cdef", 2)` = `"abcdef"`

---

## Main Algorithm: GreedySCS(fragments)

```text
RemoveDominated(fragments)

WHILE fragments.size > 1 DO

    maxOverlap = -1
    bestFirst = null
    bestSecond = null
    bestMerged = null

    FOR each A in fragments DO
        FOR each B in fragments DO

            IF A is B THEN
                CONTINUE

            overlap = CalculateOverlap(A, B)

            IF overlap > maxOverlap THEN
                maxOverlap = overlap
                bestFirst = A
                bestSecond = B
                bestMerged = Merge(A, B, overlap)

    REMOVE bestFirst from fragments
    REMOVE bestSecond from fragments

    ADD bestMerged to fragments

RETURN the only string left in fragments
```

---

## Worked Example

Input fragments: `["abcd", "cdef", "efgh"]`

| Round | Pair chosen | Overlap | Result |
|-------|-------------|---------|--------|
| 1 | `"abcd"` + `"cdef"` | 2 (`cd`) | `"abcdef"` |
| 2 | `"abcdef"` + `"efgh"` | 2 (`ef`) | `"abcdefgh"` |

Final output: `"abcdefgh"` (length 8)

---

## Why Greedy Is Not Always Optimal

Counter-example: `["ABC", "BCD", "CDA", "DAB"]`

- Greedy output: `"ABCDAB"` (length 6)
- Optimal output: `"ABCDA"` (length 5)

Greedy picks the locally best merge at each step, but this can block a better global arrangement.