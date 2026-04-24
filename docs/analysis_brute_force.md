# Brute-Force Code Analysis (Task C)

## I/O Format

**Input:** a plain text file, one fragment per line, no blank lines.
- Pass filename as `argv[1]`, or `-` to read from stdin.
- Example:
  ```
  abc
  bcd
  cde
  ```

**Output:**
- stdout: every shortest superstring found (one per line, no newline added — `fputs` is used)
- stderr: progress messages + final count/length summary

---

## Data Structure: `struct fragment_s` (singly linked list)

```c
struct fragment_s {
  struct fragment_s * next_fragment;  // pointer to next node
  char * fragment_string;             // heap-allocated string (from getline)
};
```

Fragments are prepended (not appended), so the list is in **reverse input order**.
This does not affect correctness since all fragments must be matched regardless of order.

Memory ownership: `fragment_string` is allocated by `getline()` and must be `free()`d by the caller — handled in `free_all_fragments()`.

---

## Key Functions

### `read_all_fragments(file_name)`
- Opens file (or stdin if `-`)
- Calls `getline()` in a loop — getline allocates the buffer itself
- Strips `\n` / `\r` by replacing with `\0`
- Prepends each fragment to the linked list
- Returns head of list

**Reusable as-is** in our own algorithm — just call it the same way.

### `brute_force_string_matching(length, corpus, pattern)`
- Naive O(length × |pattern|) substring search
- Returns `true` if `pattern` appears anywhere in `corpus[0..length-1]`
- Allows zero-length pattern matches (the `i <= length` condition)

**Can be reused** for validation. For our algorithm we may want a faster overlap-computation function instead.

### `iterate_next_string_of_given_size(length, string)`
- Treats the string as a base-256 counter, increments from the left
- Returns `false` when all 256^length strings have been enumerated
- This is the bottleneck — **do not reuse**, this is what makes brute-force slow

### `free_all_fragments(top_fragment)`
- Walks the list, frees both the string and the node
- **Reuse directly** in our implementation

---

## Algorithm Flow

```
search_length = 0
while no solution found:
    for every string S of length search_length (256^search_length candidates):
        if every fragment is a substring of S:
            print S, solutions++
    search_length++
```

**Why it's correct:** it tries every possible string in length order, so the first solutions found are guaranteed shortest.

**Why it's slow:** search space is 256^L where L is the answer length. Even L=6 means ~281 billion candidates.

---

## What We Can Reuse

| Component | Reuse? | Notes |
|-----------|--------|-------|
| `struct fragment_s` | Yes | same linked list structure |
| `read_all_fragments()` | Yes | identical I/O format |
| `free_all_fragments()` | Yes | same memory model |
| `brute_force_string_matching()` | Yes (validation) | too slow for inner loop of our algorithm |
| `iterate_next_string_of_given_size()` | No | replace with smarter search |

---

## Overlap Computation (needed for our algorithm)

Instead of enumerating strings, our algorithm needs to compute how much two fragments overlap:

```c
// returns the length of the longest suffix of `a` that is a prefix of `b`
int overlap(const char *a, const char *b) {
    int la = strlen(a), lb = strlen(b);
    int max_overlap = (la < lb) ? la : lb;
    for (int k = max_overlap; k > 0; k--) {
        if (strncmp(a + la - k, b, k) == 0)
            return k;
    }
    return 0;
}
```

This replaces the role of `brute_force_string_matching` in our algorithm.
