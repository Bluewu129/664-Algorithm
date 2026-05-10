#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

void held_karp(char *result, int *scs_len);
void reconstruct(char *result);
void print_overlap_matrix(void);
void verify_result(const char *result);

/* ===== SHARED PREPROCESSING (Role C) -- BEGIN =====
   Pasted verbatim from preprocessing/preprocess_canonical.c.
   Owns: input loading, substring elimination, overlap matrix.
   When fixing here, mirror the change in the canonical file AND in
   src/greedy_scs.c in the same commit. */

typedef struct {
    char **frag;
    int   *len;
    int    n;
    int    cap;
} Fragments;

typedef struct {
    int **m;
    int   n;
} OverlapMatrix;

static long pp_read_line(char **out, FILE *in) {
    size_t cap = 128, len = 0;
    char  *buf = (char *)malloc(cap);
    if (!buf) return -1;
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            char *grown = (char *)realloc(buf, cap);
            if (!grown) { free(buf); return -1; }
            buf = grown;
        }
        buf[len++] = (char)c;
        if (c == '\n') break;
    }
    if (len == 0) { free(buf); return -1; }
    buf[len] = '\0';
    *out = buf;
    return (long)len;
}

static void frags_push(Fragments *f, char *s) {
    if (f->n == f->cap) {
        f->cap  = f->cap ? f->cap * 2 : 16;
        f->frag = (char **)realloc(f->frag, f->cap * sizeof(char *));
        f->len  = (int   *)realloc(f->len,  f->cap * sizeof(int));
        assert(f->frag && f->len);
    }
    f->frag[f->n] = s;
    f->len [f->n] = (int)strlen(s);
    f->n++;
}

Fragments load_fragments(const char *path) {
    Fragments f = { NULL, NULL, 0, 0 };
    FILE *in = stdin;
    if (strcmp(path, "-") != 0) {
        in = fopen(path, "r");
        if (!in) {
            fprintf(stderr, "Error: cannot open %s\n", path);
            exit(1);
        }
    }
    char *line = NULL;
    long  r;
    while ((r = pp_read_line(&line, in)) > 0) {
        while (r > 0 && (line[r-1] == '\n' || line[r-1] == '\r'))
            line[--r] = '\0';
        if (r == 0) { free(line); line = NULL; continue; }
        frags_push(&f, line);
        line = NULL;
    }
    if (in != stdin) fclose(in);
    return f;
}

void free_fragments(Fragments *f) {
    if (!f) return;
    for (int i = 0; i < f->n; i++) free(f->frag[i]);
    free(f->frag);
    free(f->len);
    f->frag = NULL; f->len = NULL; f->n = 0; f->cap = 0;
}

int prune_substrings(Fragments *f) {
    int nn = f->n;
    if (nn == 0) return 0;
    char *remove = (char *)calloc(nn, 1);
    assert(remove);
    for (int i = 0; i < nn; i++) {
        for (int j = 0; j < nn; j++) {
            if (i == j || remove[j]) continue;
            if (strstr(f->frag[j], f->frag[i]) != NULL) {
                if (f->len[i] <  f->len[j] ||
                   (f->len[i] == f->len[j] && i > j)) {
                    remove[i] = 1;
                    break;
                }
            }
        }
    }
    int removed = 0, w = 0;
    for (int i = 0; i < nn; i++) {
        if (remove[i]) {
            free(f->frag[i]);
            removed++;
        } else {
            f->frag[w] = f->frag[i];
            f->len [w] = f->len [i];
            w++;
        }
    }
    f->n = w;
    free(remove);
    return removed;
}

OverlapMatrix compute_overlaps(const Fragments *f) {
    OverlapMatrix o;
    o.n = f->n;
    o.m = (int **)malloc((o.n ? o.n : 1) * sizeof(int *));
    assert(o.m);
    for (int i = 0; i < o.n; i++) {
        o.m[i] = (int *)calloc(o.n, sizeof(int));
        assert(o.m[i]);
    }
    for (int i = 0; i < o.n; i++) {
        for (int j = 0; j < o.n; j++) {
            if (i == j) continue;
            int kmax = (f->len[i] < f->len[j] ? f->len[i] : f->len[j]) - 1;
            int k;
            for (k = kmax; k > 0; k--) {
                if (memcmp(f->frag[i] + f->len[i] - k, f->frag[j], k) == 0) break;
            }
            o.m[i][j] = k;
        }
    }
    return o;
}

void free_overlap_matrix(OverlapMatrix *o) {
    if (!o || !o->m) return;
    for (int i = 0; i < o->n; i++) free(o->m[i]);
    free(o->m);
    o->m = NULL; o->n = 0;
}

/* ===== SHARED PREPROCESSING (Role C) -- END ===== */


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
 *   result  -- output buffer (must hold at least n*MAX_LEN bytes)
 *   scs_len -- output: length of the shortest common superstring
 *
 * Time complexity : O(n^2 * 2^n)
 * Space complexity: O(n * 2^n)
 */
void held_karp(char *result, int *scs_len) {
    int states    = 1 << n;
    int full_mask = states - 1;

    /* Step 1: Initialise entire DP table to NEG_INF ("unreachable"). */
    for (int mask = 0; mask < states; mask++)
        for (int i = 0; i < n; i++) {
            dp[mask][i]          = NEG_INF;
            parent_mask[mask][i] = -1;
            parent_last[mask][i] = -1;
        }

    /* Step 2: Base cases -- single-fragment subsets have 0 overlap. */
    for (int i = 0; i < n; i++)
        dp[1 << i][i] = 0;

    /* Step 3: Fill the table by iterating over all masks in order. */
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

    /* Step 4: Find the terminal state with maximum total overlap. */
    int max_overlap = 0;
    for (int i = 0; i < n; i++)
        if (dp[full_mask][i] != NEG_INF && dp[full_mask][i] > max_overlap)
            max_overlap = dp[full_mask][i];

    // SCS length = sum of all fragment lengths - total overlap saved.
    int total_len = 0;
    for (int i = 0; i < n; i++) total_len += frag_len[i];
    *scs_len = total_len - max_overlap;

    /* Step 5: Reconstruct the actual superstring. */
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
 *   result -- output buffer (written in-place; must be large enough).
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

    /* Reverse path so it reads first -> last. */
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
 *   result -- the superstring to check against.
 */
void verify_result(const char *result) {
    printf("\n[Verification]\n");
    int all_ok = 1;
    for (int i = 0; i < n; i++) {
        int found = (strstr(result, fragments[i]) != NULL);
        printf("  Fragment %d (%s): %s\n",
               i, fragments[i], found ? "FOUND" : "MISSING");
        if (!found) all_ok = 0;
    }
    printf("\n  Overall: %s\n",
           all_ok ? "All fragments verified"
                  : "ERROR -- some fragments are missing");
}

/*
 * Main driver:
 *   1. Read fragments from a file path (or '-' for stdin) via the shared
 *      preprocessing layer (Role C).
 *   2. Strip dominated (substring) fragments via the shared layer.
 *   3. Build the overlap matrix via the shared layer.
 *   4. Sync the results into the static fragments[][], frag_len[],
 *      overlap_matrix[][] and n that the DP code below expects.
 *   5. Run Held-Karp DP. Print and verify the result. (Unchanged.)
 */
int main(int argc, char *argv[]) {
    printf("=== Held-Karp Shortest Common Superstring ===\n\n");

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file | ->\n", argv[0]);
        return 1;
    }

    /* --- Shared preprocessing (Role C) --- */
    Fragments f = load_fragments(argv[1]);
    if (f.n == 0) {
        fprintf(stderr, "No fragments read.\n");
        free_fragments(&f);
        return 1;
    }
    int removed = prune_substrings(&f);
    if (removed > 0)
        printf("[Info] Removed %d dominated fragment(s) (substrings of others).\n",
               removed);

    /* Sync into the static arrays the DP code below expects. */
    if (f.n > MAX_N) {
        fprintf(stderr, "Error: n=%d exceeds MAX_N=%d.\n", f.n, MAX_N);
        free_fragments(&f);
        return 1;
    }
    n = f.n;
    for (int i = 0; i < n; i++) {
        if (f.len[i] >= MAX_LEN) {
            fprintf(stderr,
                "Error: fragment[%d] length %d exceeds MAX_LEN=%d.\n",
                i, f.len[i], MAX_LEN);
            free_fragments(&f);
            return 1;
        }
        strcpy(fragments[i], f.frag[i]);
        frag_len[i] = f.len[i];
    }

    OverlapMatrix o = compute_overlaps(&f);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            overlap_matrix[i][j] = o.m[i][j];
    free_overlap_matrix(&o);
    free_fragments(&f);
    /* --- End shared preprocessing --- */

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
