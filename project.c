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

#define KILL(...)                                                \
    {                                                            \
        fprintf(stderr, "[ERROR] %s:%d | ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                            \
        fputc('\n', stderr);                                     \
        exit(EXIT_FAILURE);                                      \
        __builtin_unreachable();                                 \
    }

#ifndef NDEBUG

#define WARN(...)                                                \
    {                                                            \
        fprintf(stderr, "[WARN]  %s:%d | ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                            \
        fputc('\n', stderr);                                     \
    }

#define LOG(...)                                                 \
    {                                                            \
        fprintf(stderr, "[LOG]   %s:%d | ", __FILE__, __LINE__); \
        fprintf(stderr, __VA_ARGS__);                            \
        fputc('\n', stderr);                                     \
    }

#endif  // NDEBUG

#define PATTERN_SIZE (512)
#define TEXT_SIZE (4096)
#define LINE_SIZE (4096)

static inline int64_t min_i64(int64_t const a, int64_t const b) {
    return (a < b) ? a : b;
}
static inline int64_t max_i64(int64_t const a, int64_t const b) {
    return (a > b) ? a : b;
}

// util functions for memory management
//

// checked malloc
void *_priv_xmalloc(char const *file, int lineno, size_t size) {
    void *ptr = malloc(size);
    if (ptr == NULL && size != 0) {
        fprintf(stderr, "[ERROR] %s:%d | xmalloc failed: %s\n", file, lineno,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return ptr;
}

// checked realloc
void *_priv_xrealloc(char const *file, int lineno, void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (size != 0 && new_ptr == NULL && ptr != NULL) {
        fprintf(stderr, "[ERROR] %s:%d | xrealloc failed: %s\n", file, lineno,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return new_ptr;
}

// checked calloc
void *_priv_xcalloc(char const *file, int lineno, size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL && size != 0) {
        fprintf(stderr, "[ERROR] %s:%d | xcalloc failed: %s\n", file, lineno,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return ptr;
}

#define xmalloc(size) _priv_xmalloc(__FILE__, __LINE__, size)
#define xrealloc(ptr, size) _priv_xrealloc(__FILE__, __LINE__, ptr, size)
#define xcalloc(nmemb, size) _priv_xcalloc(__FILE__, __LINE__, nmemb, size)

/// we define a better assert than that from the stdilb
///
#ifndef NDEBUG
#define assert(pred, ...)                                                 \
    if (!(pred)) {                                                        \
        fprintf(stderr, "[ASSERT] %s:%d | %s | %s\n", __FILE__, __LINE__, \
                #pred, __VA_ARGS__);                                      \
        exit(EXIT_FAILURE);                                               \
    }
#else
#define assert(pred, ...)        \
    if (!(pred)) {               \
        __builtin_unreachable(); \
    }
#endif  // NDEBUG

/// dna_t
///
typedef enum { DNA_A, DNA_C, DNA_T, DNA_G } dna_t;
#define DNA_SIGMA_SIZE (4)  // size of the alphabet

/// conversion functions
///
static inline ssize_t dna_to_int(dna_t const d) {
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

static inline dna_t char_to_dna(int const ch) {
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

/// readline_into_buffer: read a line (with exponentially larger buffer size),
/// comming from an initial buffer
///
/// @param stream          (in)     : stream to read
///
/// @param buffer          (in, out): not NULL
///
/// @param buffer_capacity (in, out): capacity of the buffer. not NULL
///
/// @return     number of characters read (excluding null byte);
///
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

#ifndef NDEBUG
/// readline_into_buffer_str: read a line (with exponentially larger buffer
/// size), comming from an initial buffer; store the original string
///
/// @param stream          (in)     : stream to read
///
/// @param str             (in, out): not NULL
///
/// @param buffer          (in, out): not NULL
///
/// @param buffer_capacity (in, out): capacity of the buffer. not NULL
///
/// @return     number of characters read (excluding null byte);
///
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
            str = xrealloc(buffer, (size_t)capacity * sizeof(char));
            buffer = xrealloc(buffer, (size_t)capacity * sizeof(dna_t));
        }

        str[size] = (char)ch;
        buffer[size++] = char_to_dna(ch);
        ch = fgetc(stream);
    }

    *in_str = str;
    *in_buffer = buffer;
    *in_buffer_capacity = capacity;
    return size;
}
#endif  // NDEBUG

/// pattern type
///
typedef struct {
    dna_t *p_pat;
#ifndef NDEBUG
    char *p_str;
#endif  // NDEBUG
    ssize_t p_size;
    ssize_t p_capacity;
} pattern_t;

static int pattern_create(pattern_t *pat, FILE *stream) {
    pat->p_capacity = PATTERN_SIZE;
    pat->p_pat = xmalloc((size_t)pat->p_capacity * sizeof(dna_t));
#ifdef NDEBUG
    pat->p_size = readline_into_buffer(stream, &pat->p_pat, &pat->p_capacity);
#else
    pat->p_str = xmalloc((size_t)pat->p_capacity * sizeof(char));
    pat->p_size = readline_into_buffer_str(stream, &pat->p_str, &pat->p_pat,
                                           &pat->p_capacity);
#endif  // NDEBUG

    return 0;
}

// reuse a pattern; throws away precomputed values but lets salvages the buffer
// the pattern must be constructed
static int pattern_reuse(pattern_t *pat, FILE *stream) {
#ifdef NDEBUG
    pat->p_size = readline_into_buffer(stream, &pat->p_pat, &pat->p_capacity);
#else
    pat->p_size = readline_into_buffer_str(stream, &pat->p_str, &pat->p_pat,
                                           &pat->p_capacity);
#endif  // NDEBUG
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
#endif  // NDEBUG
    pat->p_size = 0;
    pat->p_capacity = 0;

    return 0;
}

static inline size_t pattern_preffix_size(pattern_t const *pat) {
    return (size_t)pat->p_size;
}
// assumption: preffix must has pat->p_size els
//
static int pattern_preffix(pattern_t const *pat, ssize_t *preffix) {
    preffix[0] = 0;
    ssize_t k = 0;

    for (ssize_t q = 1; q < pat->p_size; ++q) {
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

// assumption: preffix must has pat->p_size els
//
static int pattern_reverse_preffix(pattern_t const *pat, ssize_t *preffix) {
    preffix[0] = 0;
    ssize_t k = 0;

    for (ssize_t q = 1; q < pat->p_size; ++q) {
        while (k > 0 && pat->p_pat[pat->p_size - 1 - k] !=
                            pat->p_pat[pat->p_size - 1 - q]) {
            k = preffix[k - 1];
        }

        if (pat->p_pat[pat->p_size - 1 - k] ==
            pat->p_pat[pat->p_size - 1 - q]) {
            ++k;
        }

        preffix[q] = k;
    }
    return 0;
}

// assumption: lambda has the size of the alphabet
//
static inline int pattern_last_occurrence(pattern_t const *pat,
                                          ssize_t *lambda) {
    for (ssize_t i = 0; i < DNA_SIGMA_SIZE; i++) {
        lambda[i] = 0;
    }

    for (ssize_t i = 0; i < pat->p_size; ++i) {
        lambda[dna_to_int(pat->p_pat[i])] = i;
    }

    return 0;
}

static inline size_t pattern_good_suffix_size(pattern_t const *pat) {
    return (size_t)pat->p_size + 1;
}
// assumption: good_suffix has size (pat->p_size + 1)
//
static int pattern_good_suffix(pattern_t const *pat, ssize_t const *preffix,
                               ssize_t *gamma) {
    ssize_t *reverse_preffix =
        xmalloc(pattern_preffix_size(pat) * sizeof(ssize_t));
    pattern_reverse_preffix(pat, reverse_preffix);

    for (ssize_t i = 0; i < pat->p_size + 1; ++i) {
        gamma[i] = pat->p_size - preffix[pat->p_size - 1];
    }

    for (ssize_t i = 1; i < pat->p_size; ++i) {
        ssize_t const j = pat->p_size - 1 - reverse_preffix[i];
        gamma[j] = min_i64(gamma[j], i - reverse_preffix[i]);
        assert(gamma[j] >= 0, "suffix needs to be nonnegative");
    }

    free(reverse_preffix);
    return 0;
}

/* -------------------------------------------------------------------------*/

/// text type
///
typedef struct {
    dna_t *t_text;
#ifndef NDEBUG
    char *t_str;
#endif  // NDEBUG
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
    text->t_str = xmalloc((size_t)text->t_capacity * sizeof(char));
    text->t_size = readline_into_buffer_str(stream, &text->t_str, &text->t_text,
                                            &text->t_capacity);
#endif  // NDEBUG

    return 0;
}

// reuse a text; throws away precomputed values but lets salvages the buffer
// the text must be constructed already constructed
static int text_reuse(text_t *text, FILE *stream) {
#ifdef NDEBUG
    text->t_size =
        readline_into_buffer(stream, &text->t_text, &text->t_capacity);
#else
    text->t_size = readline_into_buffer_str(stream, &text->t_str, &text->t_text,
                                            &text->t_capacity);
#endif  // NDEBUG
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
#endif  // NDEBUG
    text->t_size = 0;
    text->t_capacity = 0;

    return 0;
}

static ssize_t text_compare_pattern(text_t const *text, pattern_t const *pat,
                                    ssize_t const pos) {
    assert(pos >= 0, "negative positions do not exist");
    for (ssize_t i = pat->p_size - 1; i >= 0; --i) {
        if (text->t_text[pos + i] != pat->p_pat[i]) {
            return i;
        }
    }

    return pat->p_size;
}

/// algorithms

/// naive algorithm
static void naive(text_t const *text, pattern_t const *pat) {
    assert(text->t_size > 0, "empty text not allowed");
    assert(pat->p_size > 0, "empty pattern not allowed");
    for (ssize_t i = 0; i + (pat->p_size - 1) < text->t_size; ++i) {
        if (text_compare_pattern(text, pat, i) == pat->p_size) {
            fprintf(stdout, "%zd ", i);
        }
    }

    fputc('\n', stdout);
}

/// kmp: knuth morris pratt
static void kmp(text_t const *text, pattern_t const *pat) {
    assert(text->t_size > 0, "empty text not allowed");
    assert(pat->p_size > 0, "empty pattern not allowed");
    ssize_t *preffix = xmalloc(pattern_preffix_size(pat) * sizeof(ssize_t));
    pattern_preffix(pat, preffix);

    ssize_t q = 0;
    ssize_t comparisons = 0;

    // LOG("KMP:\nt| %.*s\t [size = %zd]\np| %.*s\t [size = %zd]\n",
    //     (int)text->t_size, text->t_str, text->t_size, (int)pat->p_size,
    //     pat->p_str, pat->p_size);

    // the guard should be (i + (pat->p_size - 1) < text->t_size)
    //
    for (ssize_t i = 0; i < text->t_size; ++i) {
        // LOG("checking [%zd]:\nt| %.*s\np| %.*s\n", i,
        //     (int)min_i64(i, pat->p_size),
        //     text->t_str + max_i64(0, i + 1 - pat->p_size),
        //     (int)min_i64(i, pat->p_size), pat->p_str);
        while (q > 0 && pat->p_pat[q] != text->t_text[i]) {
            comparisons++;
            //LOG("comparisons++: %zd", comparisons);
            q = preffix[q];  // NOLINT : pattern_preffix makes sure that this is
                             // not garbage
        }

        comparisons++;  // this next comparison
        // LOG("comparisons++: %zd", comparisons);
        if (pat->p_pat[q] == text->t_text[i]) {
            q++;
        }

        if (q == pat->p_size) {
            // LOG("success  [%zd]", i + 1 - pat->p_size);
            fprintf(stdout, "%zd ", i + 1 - pat->p_size);
            if (i < text->t_size-1) { // Is it really this if?? Why not...
                comparisons++;
                //LOG("comparisons++: %zd", comparisons);
                q = preffix[q - 1];  // NOLINT : pattern_preffix makes sure that
            }
                                 // this is not garbage
        }
        //fprintf(stderr, "----------------------\n");
    }

    fprintf(stdout, "\n%zd \n", comparisons);
    free(preffix);
}

/// bm: boyer morris
static void bm(text_t const *text, pattern_t const *pat) {
    assert(text->t_size > 0, "empty text not allowed");
    assert(pat->p_size > 0, "empty pattern not allowed");
    ssize_t comparisons = 0;

    ssize_t lambda[DNA_SIGMA_SIZE];
    pattern_last_occurrence(pat, lambda);

    ssize_t *preffix = xmalloc(pattern_preffix_size(pat) * sizeof(ssize_t));
    pattern_preffix(pat, preffix);

    ssize_t *gamma = xmalloc(pattern_good_suffix_size(pat) * sizeof(ssize_t));
    pattern_good_suffix(pat, preffix, gamma);

    LOG("BM:\nt| %.*s\t [size = %zd]\np| %.*s\t [size = %zd]\n",
        (int)text->t_size, text->t_str, text->t_size, (int)pat->p_size,
        pat->p_str, pat->p_size);

    for (ssize_t i = 0; i + (pat->p_size - 1) < text->t_size;) {
        ssize_t comp = text_compare_pattern(text, pat, i);
        comparisons += (comp == pat->p_size ? comp : (pat->p_size - comp));
        LOG("comparisons++: %zd", comparisons);
        if (comp == pat->p_size) {
            fprintf(stdout, "%zd ", i);
            assert(gamma[pat->p_size] > 0, "need non 0 increment");
            i += gamma[pat->p_size];
            LOG("Success!! moved using gamma %zd", gamma[pat->p_size]);
        } else {
            assert(
                max_i64(gamma[comp],
                        comp - lambda[dna_to_int(text->t_text[i + comp])]) > 0,
                "need non 0 increment");
            i += max_i64(gamma[comp],
                         comp - lambda[dna_to_int(text->t_text[i + comp])]);
            if ( gamma[comp] > comp - lambda[dna_to_int(text->t_text[i + comp])] ) {
                LOG("moved using GAMMA %zd", gamma[comp]);

            } else if ( gamma[comp] == comp - lambda[dna_to_int(text->t_text[i + comp])] ) {
                LOG("moved using BOTH %zd", gamma[comp]);

            } else {
                LOG("moved using LAMBDA %zd", comp - lambda[dna_to_int(text->t_text[i + comp])]);

            }
        }
    }

    fprintf(stdout, "\n%zd \n", comparisons);
    free(preffix);
    free(gamma);
}

int main() {
    text_t text;
    memset(&text, 0, sizeof(text_t));
    bool text_created = false;

    pattern_t pat;
    memset(&pat, 0, sizeof(pattern_t));
    bool pattern_created = false;

    int ch;
    do {
        ch = fgetc(stdin);
        switch (ch) {
            case 'T':
                fgetc(stdin);  // drop space
                if (text_created) {
                    text_reuse(&text, stdin);
                } else {
                    text_create(&text, stdin);
                    text_created = true;
                }
                break;
            case 'N':
                fgetc(stdin);  // drop space
                if (pattern_created) {
                    pattern_reuse(&pat, stdin);
                } else {
                    pattern_create(&pat, stdin);
                    pattern_created = true;
                }
                naive(&text, &pat);
                break;
            case 'K':
                fgetc(stdin);  // drop space
                if (pattern_created) {
                    pattern_reuse(&pat, stdin);
                } else {
                    pattern_create(&pat, stdin);
                    pattern_created = true;
                }
                kmp(&text, &pat);
                break;
            case 'B':
                fgetc(stdin);  // drop space
                if (pattern_created) {
                    pattern_reuse(&pat, stdin);
                } else {
                    pattern_create(&pat, stdin);
                    pattern_created = true;
                }
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
}
