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

// static inline int64_t max_i64(int64_t const a, int64_t const b) {
//     return (a > b) ? a : b;
// }

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
static void *_priv_xcalloc__(char const *file, int lineno, size_t nmemb, size_t size) {
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
// static void *_priv_xrealloc(char const *file, int lineno, void *ptr, size_t size) {
//     void *new_ptr = realloc(ptr, size);
//     if (size != 0 && new_ptr == NULL && ptr != NULL) {
//         fprintf(stderr, "[ERROR] %s:%d | xrealloc failed: %s\n", file, lineno,
//                 strerror(errno));
//         exit(EXIT_FAILURE);
//     }

//     return new_ptr;
// }

#define xmalloc(size) _priv_xmalloc(__FILE__, __LINE__, size)
#define xcalloc(nmemb, size) _priv_xcalloc__(__FILE__, __LINE__, nmemb, size)
// #define xrealloc(ptr, size) _priv_xrealloc(__FILE__, __LINE__, ptr, size)

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
// static ssize_t dna_to_int(dna_t const d) {
//     switch (d) {
//         case DNA_A:
//             return 0;
//         case DNA_C:
//             return 1;
//         case DNA_T:
//             return 2;
//         case DNA_G:
//             return 3;
//         default:
//             __builtin_unreachable();
//     }
// }

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
 *  @param stream    (in)     : stream to read
 *
 *  @param buffer    (in, out): not NULL
 *
 *  @param size      (in)     : size of the buffer
 *
 * @return     number of characters read (excluding null byte);
 */
static ssize_t readline_into_buffer(FILE *stream, dna_t *buffer,
                                    size_t size) {

    ssize_t i = 0;
    int32_t ch = fgetc(stream);
    while (ch != '\n' && ch != EOF && i < size) {
        buffer[i++] = char_to_dna(ch);
        ch = fgetc(stream);
    }
    return i;
}

#else /* NDEBUG not defined */
/*  readline_into_buffer_str: read a line (with exponentially larger buffer
 *  size), comming from an initial buffer; store the original string
 *
 *  @param stream    (in)     : stream to read
 *
 *  @param str       (in, out): not NULL
 *
 *  @param buffer    (in, out): not NULL
 *
 *  @param size      (in)     : size of the buffer
 *
 *  @return     number of characters read (excluding null byte);
 */
static size_t readline_into_buffer_str(FILE *stream, char *in_str,
                                        dna_t *buffer, size_t size) {

    size_t i = 0;
    int32_t ch = fgetc(stream);
    while (ch != '\n' && ch != EOF && i < size) {
        in_str[i] = (char)ch;
        buffer[i++] = char_to_dna(ch);
        ch = fgetc(stream);
    }

    in_str[i] = 0;
    return i;
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
    size_t t_size;
} text_t;

static int text_create(text_t *text, FILE *stream, size_t size) {
    text->t_size = size;
    text->t_text = xmalloc(size * sizeof(dna_t));
#ifdef NDEBUG
    readline_into_buffer(stream, &text->t_text, size);
#else
    text->t_str = xmalloc((size + 1) * sizeof(char));
    readline_into_buffer_str(stream, text->t_str, text->t_text, size);
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

typedef struct node node_t;

struct node //maybe add a list of all texts that end in this node
{
    int Ti;                             /* The value of i in Ti */
    int head;                           /* The path-label start at &(Ti[head]) */
    int sdep;                           /* String-Depth */
    node_t* child[DNA_SIGMA_SIZE];      /* Childs */
    /*node_t* brother;*/     /* brother */
    node_t* slink;                      /* Suffix link */
    node_t** hook;
};

typedef struct
{
    node_t* a;       /* node above */
    node_t* b;       /* node bellow */
    int s;          /* String-Depth */
} point_t;

static bool can_descend() {
    return false;
}

static void descend() {
    return;
}

static void add_leaf() {
    return;
}

static void suffix_link() {
    return;
}

static size_t build_generalized_sufix_tree(node_t* tree, size_t k, text_t* texts) {
    size_t i = 0, j, n = 0;
    point_t p = {NULL, NULL, 0};

    for(i = 0; i < k; i++) {  /* For every text */
        text_t* text = &texts[i];

        for(j = 0; j < text->t_size; j++) { /* For every suffix */
            while (!can_descend())
            {
                add_leaf();
                suffix_link();
            }
            descend();
        }

    }

    return n;
}

/* ------------------------------------------------------------------------- */

static text_t * parse_input(size_t * k, size_t * m) {
    size_t i = 0;
    text_t * texts = NULL;

    scanf("%zu\n", k);
    texts = xcalloc(*k, sizeof(text_t));

    for (i = 0; i < *k; i++) {
        size_t size;
        scanf("%zu ", &size);

        if (size > *m) {
            *m = size;
        }

        text_create(&texts[i], stdin, size);
    }

    return texts;
}


int main() {
    size_t i = 0, k = 0, m = 0;
    node_t* tree = NULL;
    text_t * texts = parse_input(&k, &m);

    tree = xcalloc(m * 2 + 1, sizeof(node_t));

    build_generalized_sufix_tree(tree, k, texts);

    free(tree);

    for (i = 0; i < k; i++) {
        text_delete(&texts[i]);
    }
    free(texts);

    return 0;
}
