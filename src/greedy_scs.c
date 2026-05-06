/* compile with:
   gcc -O2 -o greedy_scs greedy_scs.c
   run with:
   ./greedy_scs <input_file>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ── linked list (reused from brute-force) ─────────────────────────────── */

struct fragment_s {
    struct fragment_s *next_fragment;
    char              *fragment_string;
};

/* Portable replacement for POSIX getline(). Reads one line (up to and
 * including '\n') from `input` into a malloc'd buffer. Grows the buffer
 * as needed. Returns the number of bytes read, or -1 on EOF / error. */
static long
read_line(char **out_buf, FILE *input)
{
    size_t capacity = 128;
    size_t length   = 0;
    char  *buffer   = malloc(capacity);
    if (buffer == NULL) return -1;

    int c;
    while ((c = fgetc(input)) != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char *grown = realloc(buffer, capacity);
            if (grown == NULL) { free(buffer); return -1; }
            buffer = grown;
        }
        buffer[length++] = (char)c;
        if (c == '\n') break;
    }
    if (length == 0) { free(buffer); return -1; }
    buffer[length] = '\0';
    *out_buf = buffer;
    return (long)length;
}

struct fragment_s *
read_all_fragments(char const *file_name)
{
    FILE *input = stdin;
    if (strcmp(file_name, "-") != 0)
        input = fopen(file_name, "r");
    if (input == NULL) {
        fprintf(stderr, "Error: file could not be opened for reading: %s\n", file_name);
        return NULL;
    }
    struct fragment_s *top_fragment = NULL;
    while (1) {
        char *line_buffer = NULL;
        long  read_bytes  = read_line(&line_buffer, input);
        if (read_bytes <= 0) break;
        for (long i = 0; i < read_bytes; i++)
            if (line_buffer[i] == '\n' || line_buffer[i] == '\r')
                line_buffer[i] = '\0';
        if (line_buffer[0] == '\0') { free(line_buffer); continue; }
        struct fragment_s *new_fragment = malloc(sizeof(struct fragment_s));
        new_fragment->next_fragment   = top_fragment;
        new_fragment->fragment_string = line_buffer;
        top_fragment = new_fragment;
    }
    if (input != stdin) fclose(input);
    return top_fragment;
}

void
free_all_fragments(struct fragment_s *top_fragment)
{
    while (top_fragment != NULL) {
        struct fragment_s *this = top_fragment;
        top_fragment = this->next_fragment;
        free(this->fragment_string);
        free(this);
    }
}

/* ── fragment array helpers ─────────────────────────────────────────────── */

/* convert linked list → flat array of char*, return count */
static int
list_to_array(struct fragment_s *head, char ***out_arr)
{
    int n = 0;
    for (struct fragment_s *f = head; f != NULL; f = f->next_fragment)
        n++;
    char **arr = malloc(n * sizeof(char *));
    int i = 0;
    for (struct fragment_s *f = head; f != NULL; f = f->next_fragment) {
        size_t len = strlen(f->fragment_string) + 1;
        arr[i] = malloc(len);
        memcpy(arr[i++], f->fragment_string, len);
    }
    *out_arr = arr;
    return n;
}

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
 * Remove dominated fragments: if fragment[i] is a substring of fragment[j],
 * delete fragment[i] from the array.
 * Modifies arr[] in-place and updates *n.
 */
static void
remove_dominated(char **arr, int *n)
{
    for (int i = 0; i < *n; i++) {
        if (arr[i] == NULL) continue;
        for (int j = 0; j < *n; j++) {
            if (i == j || arr[j] == NULL) continue;
            if (strstr(arr[j], arr[i]) != NULL) {   /* arr[i] is substring of arr[j] */
                free(arr[i]);
                arr[i] = NULL;
                break;
            }
        }
    }
    /* compact: shift non-NULL entries to the front */
    int write = 0;
    for (int read = 0; read < *n; read++)
        if (arr[read] != NULL)
            arr[write++] = arr[read];
    *n = write;
}

/*
 * Greedy SCS: repeatedly merge the pair with the greatest overlap.
 * Returns a newly malloc'd superstring; caller must free().
 */
static char *
greedy_scs(char **arr, int n)
{
    remove_dominated(arr, &n);

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

    struct fragment_s *head = read_all_fragments(argv[1]);
    if (head == NULL) return 1;

    char **arr;
    int    n = list_to_array(head, &arr);
    free_all_fragments(head);

    if (n == 0) {
        fprintf(stderr, "No fragments read.\n");
        free(arr);
        return 1;
    }

    char *result = greedy_scs(arr, n);
    fputs(result, stdout);
    fputc('\n', stdout);

    free(result);
    free(arr);
    return 0;
}
