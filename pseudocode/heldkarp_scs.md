# Held-Karp Dynamic Programming for SCS

**Goal:** Find the shortest common superstring by maximizing the total overlap between fragments. Unlike Greedy, this DP approach uses state compression to explore all possible sequences, ensuring the global optimum.

| Property | Value |
|----------|-------|
| Time complexity | O(n^2 * 2^n), n = fragment count|
| Optimality | **guaranteed** — Held-Karp can get the global optimum |

---

## 1.Data Structure: DP Table

We use a 2D array dp[mask][last] to store the maximum overlap length.
mask: A bitmask representing the set of fragments used (e.g., 1011 means fragments 0, 1, and 3 are included).
last: The index of the fragment that currently ends the superstring.

---

## 2.Initialization: Starting Points
Every single fragment is a potential starting point for a superstring.
```text
// Create table with size (2^n) x n, initialized to -1
dp[1 << n][n] = -1 

FOR i = 0 TO n-1 DO
    dp[1 << i][i] = 0
    // Meaning: A set containing only fragment i has 0 overlap.
```

---

## 3. Transitions: Building the Set

We build up from smaller sets of fragments to larger ones.

```text
FOR current_mask = 1 TO (1 << n) - 1 DO
    FOR current_i = 0 TO n - 1 DO
        // Skip if fragment current_i is not in the current_mask
        IF NOT (current_mask contains current_i) THEN CONTINUE
        IF dp[current_mask][current_i] == -1 THEN CONTINUE

        // Try adding a NEW fragment 'next_j' to the sequence
        FOR next_j = 0 TO n - 1 DO
            IF current_mask contains next_j THEN CONTINUE
            
            new_mask = current_mask | (1 << next_j)
            overlap = OverlapMatrix[current_i][next_j]
            
            // Update the DP table if this new path provides more overlap
            new_val = dp[current_mask][current_i] + overlap
            dp[new_mask][next_j] = MAX(dp[new_mask][next_j], new_val)
```

---

## 4. Final Result

Once the table is filled, we look at the state where all fragments are used.

```text
full_mask = (1 << n) - 1
max_total_overlap = 0

FOR i = 0 TO n - 1 DO
    max_total_overlap = MAX(max_total_overlap, dp[full_mask][i])

total_fragment_length = SUM(length of all fragments)
Shortest_Length = total_fragment_length - max_total_overlap

RETURN Shortest_Length
```
