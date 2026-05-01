/* preprocess_demo.c — Week 9 prototype for the shared preprocessing layer.
   Implements `load_fragments`, `prune_substrings`, `compute_overlaps`
   and the matching destructors as specified in week9_preprocessing_design.md.

   Compile: gcc -O2 -o preprocess_demo preprocess_demo.c
   Run:     ./preprocess_demo tests/04-example_from_spec.txt
            cat tests/04-example_from_spec.txt | ./preprocess_demo -
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

/* ---- Data structures (matches week9_preprocessing_design.md §2) ---- */

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

/* ---- load_fragments ---- */

static void frags_push(Fragments *f, char *s) {
    if (f->n == f->cap) {
        f->cap  = f->cap ? f->cap * 2 : 16;
        f->frag = realloc(f->frag, f->cap * sizeof(char *));
        f->len  = realloc(f->len,  f->cap * sizeof(int));
        assert(f->frag && f->len);
    }
    f->frag[f->n] = s;
    f->len [f->n] = (int) strlen(s);
    f->n++;
}

Fragments load_fragments(const char *file_name) {
    Fragments f = { NULL, NULL, 0, 0 };
    FILE *in = stdin;
    if (strcmp(file_name, "-") != 0) {
        in = fopen(file_name, "r");
        if (!in) { fprintf(stderr, "Error: cannot open %s\n", file_name); exit(1); }
    }
    char *line = NULL;
    size_t cap = 0;
    ssize_t r;
    while ((r = getline(&line, &cap, in)) > 0) {
        while (r > 0 && (line[r-1] == '\n' || line[r-1] == '\r')) line[--r] = '\0';
        if (r == 0) continue;          /* skip blank lines defensively */
        char *copy = malloc(r + 1);
        assert(copy);
        memcpy(copy, line, r + 1);
        frags_push(&f, copy);
    }
    free(line);
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

/* ---- prune_substrings ---- */
/* Removes every fragment that is a substring of another fragment.
   Tie-break for identical strings: the lower-index copy survives.
   Returns the number of fragments removed. */
int prune_substrings(Fragments *f) {
    int n = f->n;
    bool *remove = calloc(n, sizeof(bool));
    assert(remove);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (i == j || remove[j]) continue;
            if (strstr(f->frag[j], f->frag[i]) != NULL) {
                if (f->len[i] < f->len[j] ||
                    (f->len[i] == f->len[j] && i > j)) {
                    remove[i] = true;
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

/* ---- compute_overlaps ----
   o.m[i][j] = longest k < min(|f_i|, |f_j|) such that
   the suffix of frag[i] of length k equals the prefix of frag[j] of length k.
   o.m[i][i] = 0 by convention. */
OverlapMatrix compute_overlaps(const Fragments *f) {
    OverlapMatrix o;
    o.n = f->n;
    o.m = malloc(o.n * sizeof(int *));
    assert(o.m);
    for (int i = 0; i < o.n; i++) {
        o.m[i] = calloc(o.n, sizeof(int));
        assert(o.m[i]);
    }
    for (int i = 0; i < o.n; i++) {
        for (int j = 0; j < o.n; j++) {
            if (i == j) { o.m[i][j] = 0; continue; }
            int kmax = (f->len[i] < f->len[j] ? f->len[i] : f->len[j]) - 1;
            int k;
            for (k = kmax; k > 0; k--) {
                if (memcmp(f->frag[i] + f->len[i] - k, f->frag[j], k) == 0) break;
            }
            o.m[i][j] = k;   /* 0 if no overlap found */
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

/* ---- smoke-test driver ---- */

static void print_fragments(const char *label, const Fragments *f) {
    printf("%s (n=%d):\n", label, f->n);
    for (int i = 0; i < f->n; i++) {
        printf("  [%2d] (len=%d) %s\n", i, f->len[i], f->frag[i]);
    }
}

static void print_matrix(const OverlapMatrix *o) {
    printf("overlap matrix (rows = from, cols = to; - on diagonal):\n");
    printf("       ");
    for (int j = 0; j < o->n; j++) printf("%4d", j);
    printf("\n");
    for (int i = 0; i < o->n; i++) {
        printf("  [%2d]", i);
        for (int j = 0; j < o->n; j++) {
            if (i == j) printf("   -");
            else        printf("%4d", o->m[i][j]);
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file | ->\n", argv[0]);
        return 1;
    }
    Fragments f = load_fragments(argv[1]);
    print_fragments("loaded fragments", &f);
    printf("\n");

    int removed = prune_substrings(&f);
    printf("prune_substrings: removed %d fragment(s)\n\n", removed);
    print_fragments("after pruning", &f);
    printf("\n");

    OverlapMatrix o = compute_overlaps(&f);
    print_matrix(&o);

    free_overlap_matrix(&o);
    free_fragments(&f);
    return 0;
}
