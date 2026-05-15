#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_N 22
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

            if (!(mask & (1 << i)))    continue;
            if (dp[mask][i] == NEG_INF) continue;

            for (int j = 0; j < n; j++) {
                if (mask & (1 << j)) continue;

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
 * Parameters:
 *   result — output buffer (written in-place; must be large enough).
 */
void reconstruct(char *result) {
    int full_mask = (1 << n) - 1;

    int best_last = 0, best_val = NEG_INF;
    for (int i = 0; i < n; i++)
        if (dp[full_mask][i] != NEG_INF && dp[full_mask][i] > best_val) {
            best_val  = dp[full_mask][i];
            best_last = i;
        }

    int path[MAX_N], path_len = 0;
    int cur_mask = full_mask;
    int cur_last = best_last;

    while (cur_mask != 0 && path_len < n) {
        path[path_len++] = cur_last;
        int prev_m = parent_mask[cur_mask][cur_last];
        int prev_l = parent_last[cur_mask][cur_last];
        if (prev_m == -1) break;
        cur_mask = prev_m;
        cur_last = prev_l;
    }

    for (int i = 0; i < path_len / 2; i++) {
        int tmp            = path[i];
        path[i]            = path[path_len - 1 - i];
        path[path_len-1-i] = tmp;
    }

    strcpy(result, fragments[path[0]]);
    for (int k = 1; k < path_len; k++) {
        int prev = path[k - 1];
        int curr = path[k];
        strcat(result, fragments[curr] + overlap_matrix[prev][curr]);
    }
}

/*
 * Main driver:
 *   1. Read fragment file path from argv[1].
 *   2. Parse the file: one fragment per line, no count line.
 *   3. Strip dominated (substring) fragments.
 *   4. Run Held-Karp DP.
 *   5. Print only the SCS string to stdout (same format as greedy_scs).
 *
 * Usage: ./held-karp <input.txt>
 *
 * File format (one fragment per line, no header):
 *   ATGCGT
 *   CGTACG
 *   TACGTA
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input.txt>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot open file '%s'\n", argv[1]);
        return 1;
    }

    n = 0;
    char line[MAX_LEN];
    while (fgets(line, sizeof(line), fp)) {
        int len = (int)strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[--len] = '\0';
        if (len == 0) continue;
        if (n >= MAX_N) {
            fprintf(stderr, "Error: too many fragments (max %d).\n", MAX_N);
            fclose(fp);
            return 1;
        }
        strcpy(fragments[n], line);
        frag_len[n] = len;
        n++;
    }
    fclose(fp);

    if (n == 0) {
        fprintf(stderr, "Error: no fragments found in '%s'.\n", argv[1]);
        return 1;
    }

    remove_substrings();
    build_overlap_matrix();

    char result[MAX_N * MAX_LEN];
    int  scs_len;
    held_karp(result, &scs_len);

    /* Output: one line, just the SCS string — identical format to greedy_scs. */
    puts(result);
    return 0;
}