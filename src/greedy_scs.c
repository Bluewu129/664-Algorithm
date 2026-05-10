/* compile with:
   gcc -O2 -o greedy_scs greedy_scs.c
   run with:
   ./greedy_scs <input_file>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ===== SHARED PREPROCESSING (Role C) -- BEGIN =====
   Pasted verbatim from preprocessing/preprocess_canonical.c.
   Owns: input loading, substring elimination, overlap matrix.
   When fixing here, mirror the change in the canonical file AND in
   src/held-karp.c in the same commit. */

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


/* ── core algorithm ─────────────────────────────────────────────────────── */

/*
 * Returns the length of the longest suffix of a that equals a prefix of b.
 * e.g. overlap("abcd", "cdef") = 2  (shared "cd")
 */
static int
calculate_overlap(const char *a, const char *b)
{
    int la = (int)strlen(a);
    int lb = (int)strlen(b);
    int max_k = la < lb ? la : lb;
    for (int k = max_k; k > 0; k--)
        if (strncmp(a + la - k, b, k) == 0)
            return k;
    return 0;
}

/*
 * Merge a and b given their known overlap.
 * Returns a newly malloc'd string; caller must free().
 */
static char *
merge(const char *a, const char *b, int overlap)
{
    int la  = (int)strlen(a);
    int lb  = (int)strlen(b);
    int len = la + lb - overlap;
    char *result = malloc(len + 1);
    memcpy(result, a, la);
    memcpy(result + la, b + overlap, lb - overlap);
    result[len] = '\0';
    return result;
}

/*
 * Greedy SCS: repeatedly merge the pair with the greatest overlap.
 * Returns a newly malloc'd superstring; caller must free().
 *
 * NOTE: dominance/substring elimination is now done by the shared
 * preprocessing layer (Role C) before this function is called, so the
 * remove_dominated() call previously at the top of this function has
 * been removed as redundant.
 */
static char *
greedy_scs(char **arr, int n)
{
    while (n > 1) {
        int   best_overlap = -1;
        int   best_i = 0, best_j = 1;
        char *best_merged = NULL;

        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i == j) continue;
                int ov = calculate_overlap(arr[i], arr[j]);
                if (ov > best_overlap) {
                    best_overlap = ov;
                    best_i       = i;
                    best_j       = j;
                    free(best_merged);
                    best_merged  = merge(arr[i], arr[j], ov);
                }
            }
        }

        /* replace arr[best_i] with merged result, remove arr[best_j] */
        free(arr[best_i]);
        arr[best_i] = best_merged;
        free(arr[best_j]);

        /* compact: shift entries after best_j left by one */
        for (int k = best_j; k < n - 1; k++)
            arr[k] = arr[k + 1];
        n--;
    }

    return arr[0];   /* caller owns this string */
}

/* ── main ───────────────────────────────────────────────────────────────── */

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [ <input_file> | - ]\n", argv[0]);
        return 1;
    }

    /* --- Shared preprocessing (Role C) --- */
    Fragments f = load_fragments(argv[1]);
    if (f.n == 0) {
        fprintf(stderr, "No fragments read.\n");
        free_fragments(&f);
        return 1;
    }
    prune_substrings(&f);

    /* Hand the (already-pruned) fragment strings off to the existing
     * (char**, n) API. We transfer ownership: arr takes the strings,
     * and free_fragments() then releases only the wrapper arrays. */
    int    n   = f.n;
    char **arr = malloc(n * sizeof(char *));
    for (int i = 0; i < n; i++) {
        arr[i]    = f.frag[i];
        f.frag[i] = NULL;          /* prevent double-free */
    }
    free_fragments(&f);
    /* --- End shared preprocessing --- */

    char *result = greedy_scs(arr, n);
    fputs(result, stdout);
    fputc('\n', stdout);

    free(result);
    free(arr);
    return 0;
}
