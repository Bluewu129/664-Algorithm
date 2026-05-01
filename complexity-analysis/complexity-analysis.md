# Complexity Analysis Notes — SIMPLE VERSION
**Task D — Week 9**

---

## Key Parameters

| Symbol | Meaning |
|--------|---------|
| `n` | Number of fragments (after removing redundant ones) |
| `L` | Total input length: Σ\|fᵢ\| |
| `ℓ` | Length of the optimal reconstruction (output length) |
| `σ` | Alphabet size (e.g. 26 for English, 256 for bytes) |

> Note: σ and ℓ do not appear in either algorithm's complexity — both algorithms only operate on precomputed overlap values and never enumerate characters.

---

## Greedy Algorithm

| | Complexity |
|---|---|
| **Time** | O(nL) — dominated by preprocessing |
| **Space** | O(n²) — overlap matrix |
| **Optimal?** | No (heuristic, approximation ratio ≤ 4) |
| **Primary parameters** | n and L |

### Step 1: Preprocessing — overlap table
- Compute max overlap for all ordered pairs (fᵢ, fⱼ) using KMP/Z-algorithm
- Each pair: O(\|fᵢ\| + \|fⱼ\|) = O(L/n) on average
- n² pairs total → **O(n²) × O(L/n) = O(nL)**
- Subsumption check (remove fragments contained in others) done in same pass

### Step 2: Greedy merge loop
- Repeat n−1 times: pick pair with maximum overlap, merge
- With priority queue: each insert/extract = O(log n²) = O(log n)
- n² operations total → **O(n² log n)**

### Overall
**Total time: O(nL + n² log n)**

Which term dominates depends on input. Typically L >> n log n so O(nL) dominates, but both terms should be kept for rigour.

---

## Held-Karp Algorithm (Exact DP)

| | Complexity |
|---|---|
| **Time** | O(n²·2ⁿ) + O(nL) preprocessing |
| **Space** | O(n·2ⁿ) |
| **Optimal?** | Yes (guaranteed) |
| **Primary parameters** | n (exponential), then L |

### Step 1: Preprocessing
- Same overlap table as Greedy: **O(nL)**

### Step 2: DP design
- State: `dp[S][i]` where S ⊆ {1,...,n} is the set of fragments placed so far, i ∈ S is the last fragment placed
- Value: shortest reconstruction length in this state
- Total states: n × 2ⁿ

### Step 3: DP transition
For each state (S, i), enumerate next fragment j ∉ S:

```
dp[S ∪ {j}][j] = min over all i ∈ S of:
    dp[S][i] + |fⱼ| − overlap(fᵢ, fⱼ)
```

- n × 2ⁿ states × O(n) transitions each → **O(n²·2ⁿ)**

### Step 4: Recover optimal solution
- Backtracking array stored during DP: O(n·2ⁿ) space
- Trace back from best final state to recover fragment ordering
- Final answer = `min over all i of dp[{f1,...,fn}][i]`

### Practical limit

| n | States (n·2ⁿ) |
|---|---|
| 10 | ~10,000 |
| 20 | ~20,000,000 |
| 25 | ~800,000,000 |
| 30 | out of memory |

Held-Karp is only feasible for **n ≲ 20–25**.

---

## Comparison

| Aspect | Greedy | Held-Karp |
|--------|--------|-----------|
| Time | O(nL + n² log n) | O(n²·2ⁿ) + O(nL) |
| Space | O(n²) | O(n·2ⁿ) |
| Optimal? | No | Yes |
| Primary parameter | L (total length) | n (fragment count) |
| Practical limit | Large n, large L | n ≲ 20–25 |

---
