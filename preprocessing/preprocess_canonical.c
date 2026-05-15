/* preprocess_canonical.c -- Canonical (paste-ready) preprocessing block.

   This file is the SINGLE SOURCE OF TRUTH for the shared preprocessing
   layer that every submitted algorithm must embed. The submission rule
   forbids #include of a shared header, so the marked block below is
   pasted verbatim into:
       src/greedy_scs.c       (Algorithm 2 - Greedy)
       src/held-karp.c        (Algorithm 1 - Bitmask DP)

   When a fix lands here, mirror it into the two algorithm files in the
   same commit. The block boundary is intentional and grep-friendly:
       SHARED PREPROCESSING (Role C) - BEGIN
       SHARED PREPROCESSING (Role C) - END

   Portability notes:
   - Uses fgetc-based pp_read_line() rather than POSIX getline(), so the
     pasted code compiles on the brute-force-style toolchain expected
     by the marker (no -std=c2x, no POSIX feature macros required).
   - All allocations grow dynamically -- no MAX_N / MAX_LEN ceilings.

   Build and smoke test (this file alone):
       gcc -O2 -o preprocess_canonical preprocess_canonical.c
       ./preprocess_canonical tests/04-example_from_spec.txt
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ===== SHARED PREPROCESSING (Role C) — BEGIN ===== */

typedef struct {
    char **frag;   /* frag[i] = malloc'd null-terminated fragment string */
    int   *len;    /* len[i]  = strlen(frag[i]) */
    int    n;      /* current number of fragments */
    int    cap;    /* allocated capacity of frag[] / len[] */
} Fragments;

typedef struct {
    int **m;       /* m[i][j] = longest k < min(|f_i|,|f_j|) with suffix(i,k) == prefix(j,k) */
    int   n;       /* m is n×n */
} OverlapMatrix;

/* Portable line reader: reads up to and including '\n' from `in`.
   Returns malloc'd buffer in *out (caller frees). Returns bytes read
   (>= 1), or -1 on EOF / error. Trailing newline is left in place;
   caller is expected to strip CR/LF. */
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

/* Read fragments from `path`. `path == "-"` reads from stdin.
   One fragment per line; blank lines and CR/LF are skipped. */
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
        frags_push(&f, line);  /* takes ownership */
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

/* Remove every fragment that is a substring of another fragment.
   Tie-break for identical strings: the lower-index copy survives.
   (This fixes the 'both copies erased' bug present in some inline
   versions.) Returns the count of fragments removed. */
int prune_substrings(Fragments *f) {
    int n = f->n;
    if (n == 0) return 0;
    char *remove = (char *)calloc(n, 1);
    assert(remove);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
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
    for (int i = 0; i < n; i++) {
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

/* Compute the n×n overlap matrix.
   o.m[i][j] = longest k with 0 <= k < min(|f_i|,|f_j|) such that
   the suffix of frag[i] of length k equals the prefix of frag[j] of length k.
   o.m[i][i] = 0 by convention. */
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

/* ===== SHARED PREPROCESSING (Role C) — END ===== */


/* ---- smoke-test driver (excluded from pasted block) ---- */

static void print_fragments(const char *label, const Fragments *f) {
    fprintf(stderr, "%s (n=%d):\n", label, f->n);
    for (int i = 0; i < f->n; i++)
        fprintf(stderr, "  [%2d] (len=%d) %s\n", i, f->len[i], f->frag[i]);
}

static void print_matrix(const OverlapMatrix *o) {
    fprintf(stderr, "overlap matrix (rows=from, cols=to; - on diagonal):\n");
    fprintf(stderr, "       ");
    for (int j = 0; j < o->n; j++) fprintf(stderr, "%4d", j);
    fprintf(stderr, "\n");
    for (int i = 0; i < o->n; i++) {
        fprintf(stderr, "  [%2d]", i);
        for (int j = 0; j < o->n; j++) {
            if (i == j) fprintf(stderr, "   -");
            else        fprintf(stderr, "%4d", o->m[i][j]);
        }
        fprintf(stderr, "\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file | ->\n", argv[0]);
        return 1;
    }
    Fragments f = load_fragments(argv[1]);
    print_fragments("loaded fragments", &f);
    int removed = prune_substrings(&f);
    fprintf(stderr, "\nprune_substrings: removed %d fragment(s)\n", removed);
    print_fragments("after pruning", &f);
    OverlapMatrix o = compute_overlaps(&f);
    print_matrix(&o);
    free_overlap_matrix(&o);
    free_fragments(&f);
    return 0;
}
