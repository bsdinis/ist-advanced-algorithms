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

#define LINE_SIZE (4096)

#if __STDC_VERSION__ == 199409L || __STRICT_ANSI__
#define WEIRD_OLD_C_OMG_WHY_DO_U_DO_DIS_2_US
#endif

#define KILL(a, b) { __builtin_unreachable(); }
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
static void *_priv_xrealloc(char const *file, int lineno, void *ptr, size_t size) {
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
                #pred, msg);                                      \
        exit(EXIT_FAILURE);                                               \
    }
#else
#define assert(pred, msg)        \
    if (!(pred)) {               \
        __builtin_unreachable(); \
    }
#endif  /* NDEBUG */

/* dna_t
 */
typedef enum { DNA_A, DNA_C, DNA_T, DNA_G } dna_t;
#define DNA_SIGMA_SIZE (4)  /* size of the alphabet */

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
            str = xrealloc(str, (size_t)(capacity+1) * sizeof(char));
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
#endif  /* NDEBUG */

/* ------------------------------------------------------------------------- */

/* text type
 */
typedef struct {
    dna_t *t_text;
#ifndef NDEBUG
    char *t_str;
#endif  /* NDEBUG */
    ssize_t t_size;
} text_t;

static int text_create(text_t *text, FILE *stream, size_t size) {
    text->t_size = size;
    text->t_text = xmalloc(size * sizeof(dna_t));
#ifdef NDEBUG
    readline_into_buffer(stream, &text->t_text, size);
#else
    text->t_str = xmalloc((size + 1) * sizeof(char));
    readline_into_buffer_str(stream, &text->t_str, &text->t_text, size);
#endif  /* NDEBUG */

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
#endif  /* NDEBUG */
    text->t_size = 0;

    return 0;
}

/* ------------------------------------------------------------------------- */

static text_t * parse_input(size_t * k) {
    size_t k = 0, i = 0;
    text_t * texts = NULL;

    scanf("%zd", &k);
    texts = xcalloc(0, sizeof(text_t));


    for (i=0; i < k; i++) {


    }

    return texts;
}


int main() {
    size_t k = 0;
    text_t * texts = parse_input(&k);

    return 0;
}
