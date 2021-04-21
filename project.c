/***
 * project.cc
 *
 * string matching problems
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if __STDC_VERSION__ == 199409L || __STRICT_ANSI__
#define WEIRD_OLD_C_OMG_WHY_DO_U_DO_DIS_2_US
#endif

#define KILL(a, b) \
    { __builtin_unreachable(); }
#define WARN(a, b)
#define LOG

#ifndef WEIRD_OLD_C_OMG_WHY_DO_U_DO_DIS_2_US
#undef KILL
#define KILL(...)                                                \
    {                                                            \
        fprintf(stderr, "[ERROR] %s:%d | ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                            \
        fputc('\n', stderr);                                     \
        exit(EXIT_FAILURE);                                      \
        __builtin_unreachable();                                 \
    }

#ifndef NDEBUG

#undef WARN
#define WARN(...)                                                \
    {                                                            \
        fprintf(stderr, "[WARN]  %s:%d | ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                            \
        fputc('\n', stderr);                                     \
    }

#undef LOG
#define LOG(...)                                                 \
    {                                                            \
        fprintf(stderr, "[LOG]   %s:%d | ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                            \
        fputc('\n', stderr);                                     \
    }

#endif /* NDEBUG */
#endif /* WEIRD_OLD_C_OMG_WHY_DO_U_DO_DIS_2_US */

#define PATTERN_SIZE (512)
#define TEXT_SIZE (4096)
#define LINE_SIZE (4096)

#ifdef WEIRD_OLD_C_OMG_WHY_DO_U_DO_DIS_2_US
#define inline
#define ssize_t int64_t
#endif

static inline int64_t max_i64(int64_t const a, int64_t const b) {
    return (a > b) ? a : b;
}

/* util functions for memory management
 */

/* checked malloc
 */
static void *_priv_xmalloc(char const *file, int lineno, size_t size) {
    void *ptr = malloc(size); /* NOLINT */
    if (ptr == NULL && size != 0) {
        fprintf(stderr, "[ERROR] %s:%d | xmalloc failed: %s\n", file, lineno,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return ptr;
}

/* checked calloc
 */
void *priv_xcalloc__(char const *file, int lineno, size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL && size != 0) {
        fprintf(stderr, "[ERROR] %s:%d | xcalloc failed: %s\n", file, lineno,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return ptr;
}

/* checked realloc
 */
static void *_priv_xrealloc(char const *file, int lineno, void *ptr,
                            size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (size != 0 && new_ptr == NULL && ptr != NULL) {
        fprintf(stderr, "[ERROR] %s:%d | xrealloc failed: %s\n", file, lineno,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return new_ptr;
}

#define xmalloc(size) _priv_xmalloc(__FILE__, __LINE__, size)
#define xcalloc(nmemb, size) priv_xcalloc__(__FILE__, __LINE__, nmemb, size)
#define xrealloc(ptr, size) _priv_xrealloc(__FILE__, __LINE__, ptr, size)

/* we define a better assert than that from the stdilb
 */
#ifndef NDEBUG
#define assert(pred, msg)                                                 \
    if (!(pred)) {                                                        \
        fprintf(stderr, "[ASSERT] %s:%d | %s | %s\n", __FILE__, __LINE__, \
                #pred, msg);                                              \
        exit(EXIT_FAILURE);                                               \
    }
#else
#define assert(pred, msg)        \
    if (!(pred)) {               \
        __builtin_unreachable(); \
    }
#endif /* NDEBUG */

/* dna_t
 */
typedef enum { DNA_A, DNA_C, DNA_T, DNA_G } dna_t;
#define DNA_SIGMA_SIZE (4) /* size of the alphabet */

/* conversion functions
 */
static ssize_t dna_to_int(dna_t const d) {
    switch (d) {
        case DNA_A:
            return 0;
        case DNA_C:
            return 1;
        case DNA_T:
            return 2;
        case DNA_G:
            return 3;
        default:
            __builtin_unreachable();
    }
}

static dna_t char_to_dna(int const ch) {
    switch (ch) {
        case 'A':
            return DNA_A;
        case 'C':
            return DNA_C;
        case 'T':
            return DNA_T;
        case 'G':
            return DNA_G;
        default:
            KILL("invalid DNA aminoacid: %c.", ch);
    }
}

#ifdef NDEBUG
/* readline_into_buffer: read a line (with exponentially larger buffer size),
 * comming from an initial buffer
 *
 * @param stream          (in)     : stream to read
 *
 * @param buffer          (in, out): not NULL
 *
 * @param buffer_capacity (in, out): capacity of the buffer. not NULL
 *
 * @return     number of characters read (excluding null byte);
 */
static ssize_t readline_into_buffer(FILE *stream, dna_t **in_buffer,
                                    ssize_t *in_buffer_capacity) {
    dna_t *buffer = *in_buffer;
    ssize_t capacity = *in_buffer_capacity;

    ssize_t size = 0;
    int32_t ch = fgetc(stream);
    while (ch != '\n' && ch != EOF) {
        if (size == capacity - 1) {
            capacity *= 2;
            buffer = xrealloc(buffer, (size_t)capacity * sizeof(dna_t));
        }

        buffer[size++] = char_to_dna(ch);
        ch = fgetc(stream);
    }

    *in_buffer = buffer;
    *in_buffer_capacity = capacity;
    return size;
}

#else /* NDEBUG not defined */
/*  readline_into_buffer_str: read a line (with exponentially larger buffer
 *  size), comming from an initial buffer; store the original string
 *
 *  @param stream          (in)     : stream to read
 *
 *  @param str             (in, out): not NULL
 *
 *  @param buffer          (in, out): not NULL
 *
 *  @param buffer_capacity (in, out): capacity of the buffer. not NULL
 *
 *  @return     number of characters read (excluding null byte);
 */
static ssize_t readline_into_buffer_str(FILE *stream, char **in_str,
                                        dna_t **in_buffer,
                                        ssize_t *in_buffer_capacity) {
    char *str = *in_str;
    dna_t *buffer = *in_buffer;
    ssize_t capacity = *in_buffer_capacity;

    ssize_t size = 0;
    int32_t ch = fgetc(stream);
    while (ch != '\n' && ch != EOF) {
        if (size == capacity - 1) {
            capacity *= 2;
            str = xrealloc(str, (size_t)(capacity + 1) * sizeof(char));
            buffer = xrealloc(buffer, (size_t)capacity * sizeof(dna_t));
        }

        str[size] = (char)ch;
        buffer[size++] = char_to_dna(ch);
        ch = fgetc(stream);
    }

    str[size] = 0;
    *in_str = str;
    *in_buffer = buffer;
    *in_buffer_capacity = capacity;
    return size;
}
#endif /* NDEBUG */

/*  pattern type
 */
typedef struct {
    dna_t *p_pat;
#ifndef NDEBUG
    char *p_str;
#endif /* NDEBUG */
    ssize_t p_size;
    ssize_t p_capacity;
} pattern_t;

static int pattern_create(pattern_t *pat, FILE *stream) {
    pat->p_capacity = PATTERN_SIZE;
    pat->p_pat = xmalloc((size_t)pat->p_capacity * sizeof(dna_t));
#ifdef NDEBUG
    pat->p_size = readline_into_buffer(stream, &pat->p_pat, &pat->p_capacity);
#else
    pat->p_str = xmalloc((size_t)(pat->p_capacity + 1) * sizeof(char));
    pat->p_size = readline_into_buffer_str(stream, &pat->p_str, &pat->p_pat,
                                           &pat->p_capacity);
#endif /* NDEBUG */

    return 0;
}

/* reuse a pattern; throws away precomputed values but salvages the buffer
 * the pattern must be already `create`d
 */
static int pattern_reuse(pattern_t *pat, FILE *stream) {
#ifdef NDEBUG
    pat->p_size = readline_into_buffer(stream, &pat->p_pat, &pat->p_capacity);
#else
    pat->p_size = readline_into_buffer_str(stream, &pat->p_str, &pat->p_pat,
                                           &pat->p_capacity);
#endif /* NDEBUG */
    return 0;
}

static int pattern_delete(pattern_t *pat) {
    if (pat->p_pat != NULL) {
        free(pat->p_pat);
        pat->p_pat = NULL;
    }
#ifndef NDEBUG
    if (pat->p_str != NULL) {
        free(pat->p_str);
        pat->p_str = NULL;
    }
#endif /* NDEBUG */
    pat->p_size = 0;
    pat->p_capacity = 0;

    return 0;
}

static inline size_t pattern_preffix_size(pattern_t const *pat) {
    return (size_t)pat->p_size;
}
/* assumption: preffix must has pat->p_size els
 */
static int pattern_preffix(pattern_t const *pat, ssize_t *preffix) {
    ssize_t k = 0;
    ssize_t q = 1;

    preffix[0] = 0;
    for (q = 1; q < pat->p_size; ++q) {
        while (k > 0 && pat->p_pat[k] != pat->p_pat[q]) {
            k = preffix[k - 1];
        }

        if (pat->p_pat[k] == pat->p_pat[q]) {
            ++k;
        }

        preffix[q] = k;
    }
    return 0;
}

/* Safety: last_occur has the size of (at least) |alphabet|
 */
static inline int pattern_bad_character(pattern_t const *pat,
                                        ssize_t *last_occur) {
    ssize_t i = 0;
    for (i = 0; i < DNA_SIGMA_SIZE; i++) {
        last_occur[i] = -1;
    }

    for (i = 0; i < pat->p_size; ++i) {
        last_occur[dna_to_int(pat->p_pat[i])] = i;
    }

    return 0;
}

/**
 * Compute shift array for Boyer-Moore
 * Safety: `shifts` has p_size + 1 elements
 * Correctness: `shifts` is initialized at 0
 */
static int pattern_good_suffix(pattern_t const *pat, ssize_t *shifts) {
    /* A border is a substring which is simultaneously a proper suffix and a
     * proper preffix.
     *
     * Given i, border_pos is the starting position of the proper suffix of
     * P[i..] (note: this is a suffix of a suffix) which defines the widest
     * border.
     *
     * If there is no border, the empty string is the border, so border_pos =
     * len(P)
     */
    ssize_t *border_pos = xcalloc((size_t)pat->p_size + 1, sizeof(ssize_t));
    ssize_t i = pat->p_size;
    ssize_t j = pat->p_size + 1;

    border_pos[i] = j;

    /* Construct the border positions
     * If at somepoint we cannot extend a border to the left
     * (we are inside the inner loop) and
     * there is no shift, shift by that ammount.
     */
    while (i > 0) {
        while (j < pat->p_size + 1 && pat->p_pat[i - 1] != pat->p_pat[j - 1]) {
            if (shifts[j] == 0) {
                shifts[j] = j - i;
            }

            j = border_pos[j];
        }

        i--;
        j--;
        border_pos[i] = j;
    }

    for (i = 0, j = border_pos[0]; i < pat->p_size + 1; ++i) {
        if (shifts[i] == 0) {
            shifts[i] = j;
        }

        if (i == j) {
            j = border_pos[j];
        }
    }

    free(border_pos);
    return 0;
}

/* -------------------------------------------------------------------------*/

/* text type
 */
typedef struct {
    dna_t *t_text;
#ifndef NDEBUG
    char *t_str;
#endif /* NDEBUG */
    ssize_t t_size;
    ssize_t t_capacity;
} text_t;

static int text_create(text_t *text, FILE *stream) {
    text->t_capacity = TEXT_SIZE;
    text->t_text = xmalloc((size_t)text->t_capacity * sizeof(dna_t));
#ifdef NDEBUG
    text->t_size =
        readline_into_buffer(stream, &text->t_text, &text->t_capacity);
#else
    text->t_str = xmalloc((size_t)(text->t_capacity + 1) * sizeof(char));
    text->t_size = readline_into_buffer_str(stream, &text->t_str, &text->t_text,
                                            &text->t_capacity);
#endif /* NDEBUG */

    return 0;
}

/* reuse a text; throws away precomputed values but lets salvages the buffer
 * the text must be constructed already constructed
 */
static int text_reuse(text_t *text, FILE *stream) {
#ifdef NDEBUG
    text->t_size =
        readline_into_buffer(stream, &text->t_text, &text->t_capacity);
#else
    text->t_size = readline_into_buffer_str(stream, &text->t_str, &text->t_text,
                                            &text->t_capacity);
#endif /* NDEBUG */
    return 0;
}

static int text_delete(text_t *text) {
    if (text->t_text != NULL) {
        free(text->t_text);
        text->t_text = NULL;
    }
#ifndef NDEBUG
    if (text->t_str != NULL) {
        free(text->t_str);
        text->t_str = NULL;
    }
#endif /* NDEBUG */
    text->t_size = 0;
    text->t_capacity = 0;

    return 0;
}

static ssize_t text_compare_pattern(text_t const *text, pattern_t const *pat,
                                    ssize_t const pos) {
    ssize_t i;
    assert(pos >= 0, "negative positions do not exist");
    for (i = pat->p_size - 1; i >= 0; --i) {
        if (text->t_text[pos + i] != pat->p_pat[i]) {
            return i;
        }
    }

    return pat->p_size;
}

/***
 * algorithms
 */

/**
 * naive algorithm
 */
static void naive(text_t const *text, pattern_t const *pat) {
    ssize_t i;
    assert(text->t_size > 0, "empty text not allowed");
    assert(pat->p_size > 0, "empty pattern not allowed");
    for (i = 0; i + (pat->p_size - 1) < text->t_size; ++i) {
        if (text_compare_pattern(text, pat, i) == pat->p_size) {
            fprintf(stdout, "%zd ", i);
        }
    }

    fputc('\n', stdout);
}

/**
 * kmp: knuth morris pratt
 */
static void kmp(text_t const *text, pattern_t const *pat) {
    ssize_t *preffix = NULL;
    ssize_t q = -1;
    ssize_t comparisons = 0;
    ssize_t i;

    assert(text->t_size > 0, "empty text not allowed");
    assert(pat->p_size > 0, "empty pattern not allowed");

    preffix = xmalloc(pattern_preffix_size(pat) * sizeof(ssize_t));
    pattern_preffix(pat, preffix);

    for (i = 0; i < text->t_size; ++i) {
        while (q >= 0 && pat->p_pat[q + 1] != text->t_text[i]) {
            comparisons++;
            q = preffix[q] - 1; /* NOLINT : pattern_preffix makes sure that this
                                 * is not garbage */
        }

        comparisons++; /* this next comparison */
        if (pat->p_pat[q + 1] == text->t_text[i]) {
            q++;
        }

        if (q + 1 == pat->p_size) {
            fprintf(stdout, "%zd ", i + 1 - pat->p_size);
            if (i < text->t_size - 1) {
                comparisons++;
                q = preffix[q] -
                    1; /* NOLINT : pattern_preffix makes sure that */
            }
        }
    }

    fprintf(stdout, "\n%zd \n", comparisons);
    free(preffix);
}

/**
 * bm: boyer morris
 */
static void bm(text_t const *text, pattern_t const *pat) {
    ssize_t comparisons = 0;
    ssize_t i = 0;

    ssize_t last_occur[DNA_SIGMA_SIZE];
    ssize_t *shifts = xcalloc((size_t)pat->p_size + 1, sizeof(ssize_t));

    assert(text->t_size > 0, "empty text not allowed");
    assert(pat->p_size > 0, "empty pattern not allowed");

    pattern_bad_character(pat, last_occur);
    pattern_good_suffix(pat, shifts);

    while (i <= text->t_size - pat->p_size) {
        ssize_t j = pat->p_size - 1;
        while (j >= 0 && pat->p_pat[j] == text->t_text[i + j]) {
            comparisons++;
            j--;
        }

        if (j < 0) {
            fprintf(stdout, "%zd ", i);
            i += shifts[0];
        } else {
            i += max_i64(shifts[j + 1],
                         j - last_occur[dna_to_int(text->t_text[i + j])]);
            comparisons++;
        }
    }

    fprintf(stdout, "\n%zd \n", comparisons);
    free(shifts);
}

int main() {
    int ch = 0;
    ;
    text_t text;
    bool text_created = false;

    pattern_t pat;
    bool pattern_created = false;

    memset(&text, 0, sizeof(text_t));
    memset(&pat, 0, sizeof(pattern_t));

    do {
        ch = fgetc(stdin);
        switch (ch) {
            case 'T':
                fgetc(stdin); /* drop space */
                if (text_created) {
                    text_reuse(&text, stdin);
                } else {
                    text_create(&text, stdin);
                    text_created = true;
                }
                /*LOG("processing text: %s", text.t_str);*/
                break;
            case 'N':
                fgetc(stdin); /* drop space */
                if (pattern_created) {
                    pattern_reuse(&pat, stdin);
                } else {
                    pattern_create(&pat, stdin);
                    pattern_created = true;
                }
                /*LOG("naive: %s", pat.p_str);*/
                naive(&text, &pat);
                break;
            case 'K':
                fgetc(stdin); /* drop space */
                if (pattern_created) {
                    pattern_reuse(&pat, stdin);
                } else {
                    pattern_create(&pat, stdin);
                    pattern_created = true;
                }
                /*LOG("kmp:   %s", pat.p_str);*/
                kmp(&text, &pat);
                break;
            case 'B':
                fgetc(stdin); /* drop space */
                if (pattern_created) {
                    pattern_reuse(&pat, stdin);
                } else {
                    pattern_create(&pat, stdin);
                    pattern_created = true;
                }
                /*LOG("bm:    %s", pat.p_str);*/
                bm(&text, &pat);
                break;
            case 'X':
            case EOF:
                break;
            default:
                WARN("unrecognized command `%c`", ch);
                while (fgetc(stdin) != '\n') {
                }
                break;
        }
    } while (ch != 'X' && ch != EOF);

    if (text_created) {
        text_delete(&text);
    }
    if (pattern_created) {
        pattern_delete(&pat);
    }

    return 0;
}
