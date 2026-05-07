#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_N 20
#define MAX_LEN 100
#define NEG_INF -1

// dp[mask][last] = maximum total overlap achieved using the set of fragments
// described by 'mask', ending with fragment 'last'.
int dp[1 << MAX_N][MAX_N];

// For path reconstruction: stores the previous (mask, last) state.
int parent_mask[1 << MAX_N][MAX_N];
int parent_last[1 << MAX_N][MAX_N];

char fragments[MAX_N][MAX_LEN]; // Fragment strings
int  frag_len[MAX_N];           // Length of each fragment
int  overlap_matrix[MAX_N][MAX_N]; // overlap_matrix[i][j] = overlap(frag i -> frag j)
int  n;                         // Current number of fragments

int  compute_overlap(const char *a, int la, const char *b, int lb);
int  is_substring(int i, int j);
void remove_substrings(void);
void build_overlap_matrix(void);
void held_karp(char *result, int *scs_len);
void reconstruct(char *result);
void print_overlap_matrix(void);
void verify_result(const char *result);

/*
 * Compute the overlap length between two fragments a and b.
 * Overlap is defined as the longest suffix of 'a' that matches
 * a prefix of 'b'.
 *
 * Parameters:
 *   a, la  — string a and its length
 *   b, lb  — string b and its length
 *
 * Returns: the overlap length (0 if none).
 */
int compute_overlap(const char *a, int la, const char *b, int lb) {
    int max_ov = (la < lb) ? la : lb;
    for (int ov = max_ov; ov > 0; ov--) {
        if (memcmp(a + la - ov, b, ov) == 0)
            return ov;
    }
    return 0;
}

/*
 * Check whether fragment[i] is a proper substring of fragment[j].
 * A fragment is not considered a substring of itself (i == j → false).
 *
 * Returns: 1 if fragment[i] ⊆ fragment[j], 0 otherwise.
 */
int is_substring(int i, int j) {
    if (i == j)                    return 0;
    if (frag_len[i] > frag_len[j]) return 0;
    return strstr(fragments[j], fragments[i]) != NULL;
}

/*
 * Remove dominated fragments in-place.
 * If fragment[i] is a substring of any fragment[j] (i ≠ j), fragment[i]
 * is redundant and can never contribute new characters to the superstring.
 * Such fragments are removed by compacting the global fragments[] array
 * and decrementing n.
 *
 * Modifies globals: fragments[], frag_len[], n.
 */
void remove_substrings(void) {
    int dominated[MAX_N] = {0};

    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            if (!dominated[i] && is_substring(i, j))
                dominated[i] = 1;

    // Compact the array, keeping only non-dominated fragments.
    int k = 0;
    for (int i = 0; i < n; i++) {
        if (!dominated[i]) {
            if (k != i) {
                strcpy(fragments[k], fragments[i]);
                frag_len[k] = frag_len[i];
            }
            k++;
        }
    }

    int removed = n - k;
    if (removed > 0)
        printf("[Info] Removed %d dominated fragment(s) (substrings of others).\n",
               removed);
    n = k;
}

/*
 * Pre-compute the full n×n overlap matrix.
 * overlap_matrix[i][j] stores the length of the longest suffix of
 * fragment[i] that is also a prefix of fragment[j].
 * Diagonal entries (i == j) are set to 0 and never used by the DP.
 *
 * Modifies global: overlap_matrix[][].
 */
void build_overlap_matrix(void) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            overlap_matrix[i][j] = (i == j)
                ? 0
                : compute_overlap(fragments[i], frag_len[i],
                                  fragments[j], frag_len[j]);
}

/*
 * Print the overlap matrix to stdout in a readable grid format.
 * Rows = "from" fragment, Columns = "to" fragment.
 * Diagonal cells are shown as "---".
 */
void print_overlap_matrix(void) {
    printf("\n[Overlap Matrix]  (row = from, col = to)\n");
    printf("      ");
    for (int j = 0; j < n; j++) printf("  F%-2d ", j);
    printf("\n");

    for (int i = 0; i < n; i++) {
        printf("  F%-2d ", i);
        for (int j = 0; j < n; j++) {
            if (i == j) printf("  --- ");
            else        printf("  %-3d ", overlap_matrix[i][j]);
        }
        printf("\n");
    }
}

/*
 * Core Held-Karp DP over all 2^n subsets of fragments.
 *
 * State:  dp[mask][i] = maximum total overlap when the set of fragments
 *         used so far is described by bitmask 'mask', and the last
 *         appended fragment is i.
 *
 * Transition:
 *   For every state (mask, i) and every fragment j not yet in mask:
 *     dp[mask | (1<<j)][j] = max(..., dp[mask][i] + overlap[i][j])
 *
 * After filling the table, the answer is read from dp[full_mask][*].
 *
 * Parameters:
 *   result  — output buffer (must hold at least n*MAX_LEN bytes)
 *   scs_len — output: length of the shortest common superstring
 *
 * Time complexity : O(n^2 * 2^n)
 * Space complexity: O(n * 2^n)
 */
void held_karp(char *result, int *scs_len) {
    int states    = 1 << n;
    int full_mask = states - 1;

    /* ── Step 1: Initialise entire DP table to NEG_INF ("unreachable"). ── */
    for (int mask = 0; mask < states; mask++)
        for (int i = 0; i < n; i++) {
            dp[mask][i]          = NEG_INF;
            parent_mask[mask][i] = -1;
            parent_last[mask][i] = -1;
        }

    /* ── Step 2: Base cases — single-fragment subsets have 0 overlap. ── */
    for (int i = 0; i < n; i++)
        dp[1 << i][i] = 0;

    /* ── Step 3: Fill the table by iterating over all masks in order. ── */
    for (int mask = 1; mask < states; mask++) {
        for (int i = 0; i < n; i++) {

            // Fragment i must be in this mask and the state must be reachable.
            if (!(mask & (1 << i)))   continue;
            if (dp[mask][i] == NEG_INF) continue;

            // Try appending each fragment j not yet in mask.
            for (int j = 0; j < n; j++) {
                if (mask & (1 << j)) continue; // j already used

                int new_mask = mask | (1 << j);
                int new_val  = dp[mask][i] + overlap_matrix[i][j];

                if (new_val > dp[new_mask][j]) {
                    dp[new_mask][j]          = new_val;
                    parent_mask[new_mask][j] = mask;
                    parent_last[new_mask][j] = i;
                }
            }
        }
    }

    /* ── Step 4: Find the terminal state with maximum total overlap. ── */
    int max_overlap = 0;
    for (int i = 0; i < n; i++)
        if (dp[full_mask][i] != NEG_INF && dp[full_mask][i] > max_overlap)
            max_overlap = dp[full_mask][i];

    // SCS length = sum of all fragment lengths − total overlap saved.
    int total_len = 0;
    for (int i = 0; i < n; i++) total_len += frag_len[i];
    *scs_len = total_len - max_overlap;

    /* ── Step 5: Reconstruct the actual superstring. ── */
    reconstruct(result);
}

/*
 * Reconstruct the shortest common superstring by tracing parent pointers
 * back from the optimal terminal state to the single-fragment base case.
 *
 * The fragment order is stored in path[] (reversed during traceback),
 * then the final string is built by appending the non-overlapping suffix
 * of each successive fragment.
 *
 * Parameters:
 *   result — output buffer (written in-place; must be large enough).
 *
 * Reads globals: dp[][], parent_mask[][], parent_last[][], fragments[][],
 *                frag_len[], overlap_matrix[][], n.
 */
void reconstruct(char *result) {
    int full_mask = (1 << n) - 1;

    /* Find the last fragment of the optimal path. */
    int best_last = 0, best_val = NEG_INF;
    for (int i = 0; i < n; i++)
        if (dp[full_mask][i] != NEG_INF && dp[full_mask][i] > best_val) {
            best_val  = dp[full_mask][i];
            best_last = i;
        }

    /* Trace back through parent pointers to recover the full order. */
    int path[MAX_N], path_len = 0;
    int cur_mask = full_mask;
    int cur_last = best_last;

    while (cur_mask != 0 && path_len < n) {
        path[path_len++] = cur_last;
        int prev_m = parent_mask[cur_mask][cur_last];
        int prev_l = parent_last[cur_mask][cur_last];
        if (prev_m == -1) break;   // reached a base-case node
        cur_mask = prev_m;
        cur_last = prev_l;
    }

    /* Reverse path so it reads first → last. */
    for (int i = 0; i < path_len / 2; i++) {
        int tmp              = path[i];
        path[i]              = path[path_len - 1 - i];
        path[path_len-1-i]   = tmp;
    }

    /* Build the superstring: start with path[0], then append
     * only the non-overlapping suffix of each subsequent fragment. */
    strcpy(result, fragments[path[0]]);
    for (int k = 1; k < path_len; k++) {
        int prev = path[k - 1];
        int curr = path[k];
        strcat(result, fragments[curr] + overlap_matrix[prev][curr]);
    }
}

/*
 * Verify that every fragment in the global fragments[] array appears as a
 * substring of 'result'. Prints a per-fragment status line and a summary.
 *
 * Parameters:
 *   result — the superstring to check against.
 */
void verify_result(const char *result) {
    printf("\n[Verification]\n");
    int all_ok = 1;
    for (int i = 0; i < n; i++) {
        int found = (strstr(result, fragments[i]) != NULL);
        printf("  Fragment %d (%s): %s\n",
               i, fragments[i], found ? "FOUND ✓" : "MISSING ✗");
        if (!found) all_ok = 0;
    }
    printf("\n  Overall: %s\n",
           all_ok ? "All fragments verified ✓"
                  : "ERROR — some fragments are missing ✗");
}

/*
 * Main driver:
 *   1. Read fragments from stdin.
 *   2. Strip dominated (substring) fragments.
 *   3. Build the overlap matrix and display it.
 *   4. Run Held-Karp DP.
 *   5. Print and verify the result.
 */
int main(void) {
    printf("=== Held-Karp Shortest Common Superstring ===\n\n");

    printf("Enter number of fragments (max %d): ", MAX_N);
    if (scanf("%d", &n) != 1 || n <= 0 || n > MAX_N) {
        fprintf(stderr, "Invalid fragment count.\n");
        return 1;
    }
    printf("Enter fragments:\n");
    for (int i = 0; i < n; i++) {
        printf("  Fragment %d: ", i);
        scanf("%s", fragments[i]);
        frag_len[i] = (int)strlen(fragments[i]);
    }

    remove_substrings();
    build_overlap_matrix();
    print_overlap_matrix();

    char result[MAX_N * MAX_LEN];
    int  scs_len;
    held_karp(result, &scs_len);

    printf("\n[Result]\n");
    printf("  Shortest Common Superstring : %s\n", result);
    printf("  SCS Length (DP)             : %d\n", scs_len);
    printf("  SCS Length (strlen)         : %d\n", (int)strlen(result));

    verify_result(result);
    return 0;
}