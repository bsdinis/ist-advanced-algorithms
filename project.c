/***
 * project.cc
 *
 * string matching problems
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_SIZE (4096)
#define DEFAULT_VEC_SIZE (16)

#if __STRICT_ANSI__
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

#ifdef WEIRD_OLD_C_OMG_WHY_DO_U_DO_DIS_2_US
#define inline
#define ssize_t int64_t
#endif

typedef enum { false = 0, true = 1 } bool;

/* In DEBUG mode, we can print our suffix tree to a DOT representation.
 * For this, having IDs to nodes is useful
 */
#ifndef NDEBUG
static size_t NODE_ID = 0; /* NOLINT */
#endif

/**
 * Utility functions for memory management:
 *  - xmalloc ====> malloc
 *  - xrealloc ====> realloc
 *  - xcalloc ====> calloc
 *
 *  These functions check for OOM or other errors when allocating memory,
 *  exiting after printing to stderr. They are actually macros which inject
 *  the filename and line number of the failed call.
 */

/**
 * `malloc` data
 *
 * @param   file    name of the file
 * @param   lineno  line number
 * @param   size    size (in B) to be allocated
 *
 * @return  pointer to allocated block (guaranteed to be valid)
 */
void *_priv_xmalloc(char const *file, int lineno, size_t size) {
    void *ptr = malloc(size); /* NOLINT */
    if (ptr == NULL && size != 0) {
        fprintf(stderr, "[ERROR] %s:%d | xmalloc failed: %s\n", file, lineno,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return ptr;
}

/**
 * `calloc` data
 *
 * @param   file    name of the file
 * @param   lineno  line number
 * @param   nmemb   number of members in the block
 * @param   size    size (in B) of each member
 *
 * @return  pointer to allocated block (guaranteed to be valid and zeroed out)
 */
void *_priv_xcalloc__(char const *file, int lineno, size_t nmemb, size_t size) {
    void *ptr = NULL;
    if (nmemb != 0 && size != 0) {
        ptr = calloc(nmemb, size);
        if (ptr == NULL) {
            fprintf(stderr, "[ERROR] %s:%d | xcalloc failed: %s\n", file,
                    lineno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    return ptr;
}

/**
 * `realloc` data
 *
 * @param   file    name of the file
 * @param   lineno  line number
 * @param   ptr     pointer to previous allocation
 * @param   size    size (in B) of new block
 *
 * @return  pointer to allocated block (guaranteed to be valid unless this was a
 * masked free)
 */
void *_priv_xrealloc(char const *file, int lineno, void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (size != 0 && new_ptr == NULL && ptr != NULL) {
        fprintf(stderr, "[ERROR] %s:%d | xrealloc failed: %s\n", file, lineno,
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    return new_ptr;
}

#define xmalloc(size) _priv_xmalloc(__FILE__, __LINE__, size)
#define xcalloc(nmemb, size) _priv_xcalloc__(__FILE__, __LINE__, nmemb, size)
#define xrealloc(ptr, size) _priv_xrealloc(__FILE__, __LINE__, ptr, size)

/**
 * A better assert:
 *  - In debug mode,    print the assertion with file name and line number
 *  - In release mode,  introduce a __builtin_unreachable() which helps out the
 * optimizer
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
 *
 * This is defined as an integer.
 */
typedef int dna_t;
#define DNA_A (0)
#define DNA_C (1)
#define DNA_T (2)
#define DNA_G (3)
#define DNA_TERM (4)
#define DNA_SIGMA_SIZE (5) /* size of the alphabet */

/* epsilon is not part of the alphabet */
#define DNA_EPSILON (5)

/**
 * Convert a character to a DNA
 *
 * @param   ch  character (ASCII)
 *
 * @returns equivalent DNA value
 *
 * @panic   if ch is not a DNA character "ACTG"
 */
dna_t char_to_dna(int const ch) {
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

/**
 * Convert a DNA to a character
 *
 * @param   dna dna to convert
 * @returns corresponding textual representation
 */
char dna_to_char(dna_t dna) { return "ACTG$\x00"[dna]; }

#ifdef NDEBUG
/**
 * Read a line (with exponentially larger buffer size), comming from an initial
 * buffer
 *
 *  @param stream    (in)     : stream to read
 *
 *  @param buffer    (in, out): not NULL
 *
 *  @param size      (in)     : size of the buffer
 *
 * @return     number of characters read (excluding null byte);
 */
ssize_t readline_into_buffer(FILE *stream, dna_t *buffer, size_t size) {
    ssize_t i = 0;
    int32_t ch = fgetc(stream);
    while (ch != '\n' && ch != EOF && i < size) {
        buffer[i++] = char_to_dna(ch);
        ch = fgetc(stream);
    }
    return i;
}

#else /* NDEBUG not defined */
/**
 * Read a line (with exponentially larger buffer size),
 * comming from an initial buffer; store the original string
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
size_t readline_into_buffer_str(FILE *stream, char *in_str, dna_t *buffer,
                                size_t size) {
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
#endif /* NDEBUG */

/**
 * text type
 */
typedef struct {
    /* vector of DNA's */
    dna_t *t_text;

    /* size of the vector */
    size_t t_size;

    /* id of the text */
    int t_id;

#ifndef NDEBUG
    /* string representation: only in debug mode */
    char *t_str;
#endif /* NDEBUG */
} text_t;

/**
 * Create a new text
 *
 * @param   text    text struct to create
 * @param   stream  stream to read from
 * @param   size    size of the text
 * @param   id      id of the text
 *
 * @return  0       success
 */
int text_create(text_t *text, FILE *stream, size_t size, int id) {
    text->t_id = id;
    text->t_size = size + 1;
    text->t_text = xmalloc((size + 1) * sizeof(dna_t));
#ifdef NDEBUG
    readline_into_buffer(stream, &text->t_text, size);
#else
    text->t_str = xmalloc((size + 1) * sizeof(char));
    readline_into_buffer_str(stream, text->t_str, text->t_text, size);
#endif /* NDEBUG */

    /* Add the terminator */
    text->t_text[size] = DNA_TERM;

    return 0;
}

/**
 * Delete a text, freeing any resources
 *
 * @param   text    text to be deleted
 * @return  0       success
 */
int text_delete(text_t *text) {
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
    text->t_id = 0;

    return 0;
}

/***
 * A bitmask
 */
typedef struct bitmask_t {
    uint64_t *bm_ints;
    size_t bm_width;
} bitmask_t;

/**
 * Create a bitmask
 *
 * @param   bm       bitmask
 * @param   width width of the mask
 *
 * @return  0 success
 */
int bm_create(bitmask_t *bm, size_t width) {
    /*
     * 63 bits -> 1 int
     * 64 bits -> 1 int
     * 65 bits -> 2 ints
     */
    size_t n_ints = (width - 1) / 64 + 1; /* NOLINT */

    bm->bm_ints = xcalloc(n_ints, sizeof(uint64_t));
    bm->bm_width = width;
    return 0;
}

/**
 * Delete a bitmask
 *
 * @param   bm       bitmask
 * @return  0 success
 */
int bm_delete(bitmask_t *bm) {
    free(bm->bm_ints);
    bm->bm_ints = NULL;
    bm->bm_width = 0;
    return 0;
}

/**
 * Set a bit in the bitmask
 *
 * @param   bm      bitmask
 * @param   index   index to set
 *
 * @panic   if index is out of bounds (index must be < bm->bm_width)
 */
void bm_set(bitmask_t *bm, size_t index) {
    size_t int_index = index / 64; /* NOLINT */
    size_t bit_index = index % 64; /* NOLINT */
    assert(index < bm->bm_width, "out of bounds access in a bitmask");

    bm->bm_ints[int_index] |= (((uint64_t)1) << bit_index);
}

/**
 * Get a bit in the bitmask
 *
 * @param   bm      bitmask
 * @param   index   index to set
 *
 * @panic   if index is out of bounds (index must be < bm->bm_width)
 * @return  whether the bit is set
 */
bool bm_get(bitmask_t const *bm, size_t index) {
    size_t int_index = index / 64; /* NOLINT */
    size_t bit_index = index % 64; /* NOLINT */
    assert(index < bm->bm_width, "out of bounds access in a bitmask");

    return (bm->bm_ints[int_index] & (((uint64_t)1) << bit_index)) != 0;
}

/**
 * Or two bitmask
 *
 * @param   a       first bitmask (will store the result)
 * @param   b       second bitmask
 */
void bm_or(bitmask_t *a, bitmask_t *b) {
    size_t a_n_ints = (a->bm_width - 1) / 64 + 1; /* NOLINT */
    size_t b_n_ints = (b->bm_width - 1) / 64 + 1; /* NOLINT */
    size_t idx = 0;
    for (idx = 0; idx < a_n_ints && idx < b_n_ints; ++idx) {
        a->bm_ints[idx] |= b->bm_ints[idx];
    }
}

/**
 * Get number of set bits in the bitmask
 * This is the equivalent of the popcount instruction
 * Also called the Hamming weight
 *
 * @param bm        bitmask
 */
size_t bm_popcnt(bitmask_t const *bm) {
    size_t n_ints = (bm->bm_width - 1) / 64 + 1; /* NOLINT */
    size_t idx = 0;
    size_t popcnt = 0;
    for (idx = 0; idx < n_ints; ++idx) {
        popcnt += (size_t)__builtin_popcountll(bm->bm_ints[idx]);
    }

    return popcnt;
}

/***
 * Node of the suffix tree
 *
 * This node has:
 *  - a label in the range [st_start, st_end[ in the st_id string
 *  - a set of children
 *  - an (optional) suffix link
 *  - a set of active texts and corresponding start idx (ie: where the label
 * starts in that text)
 */
typedef struct st_node_t {
#ifndef NDEBUG
    /* id of this node */
    size_t st_n_id;
#endif

    /* id of the text which created the node
     *
     * NOTE: this is important because the st_start and st_end will refer to
     * this text NOTE: this will be at times optional (with value -1)
     */
    int st_id;

    /* Start offset of the path label in the `st_id` text */
    size_t st_start;

    /* End offset of the path label in the `st_id` text (one past)
     *
     * NOTE: at times this value may be optional (with value SIZE_MAX)
     */
    size_t st_end;

    /* children */
    struct st_node_t *st_children[DNA_SIGMA_SIZE];

    /* suffix link */
    struct st_node_t *st_slink;

    /* which texts are active in this node
     */
    bitmask_t st_text_ids;
} st_node_t;

int st_node_create(st_node_t *node, size_t start, size_t end, int id,
                   size_t n_texts);
int st_node_delete(st_node_t *node);
void st_node_add_text(st_node_t *node, int text_id);
size_t st_node_edge_size(st_node_t const *node, size_t default_end);
size_t st_node_label_size(st_node_t const *node);
size_t st_node_n_texts(st_node_t const *node);
bool st_node_is_leaf(st_node_t const *node);

/**
 * Create a new suffix tree node.
 *
 * @param   node      the node to be created
 * @param   start     start index in the text
 * @param   end       end index in the text (one past the end): this field is
 * optional with default value SIZE_MAX
 *
 * @param   id        id of the text which creates the node: this field is
 * optional with default value -1
 *
 * @return  0   success
 */
int st_node_create(st_node_t *node, size_t start, size_t end, int id,
                   size_t n_texts) {
#ifndef NDEBUG
    node->st_n_id = NODE_ID++;
#endif
    node->st_id = id;
    node->st_start = start;
    node->st_end = end;

    memset(node->st_children, 0, /* NOLINT */
           DNA_SIGMA_SIZE * sizeof(st_node_t *));
    node->st_slink = NULL;

    if (bm_create(&node->st_text_ids, n_texts) != 0) {
        return -1;
    }

    st_node_add_text(node, id);

    return 0;
}

/**
 * Delete a suffix tree node
 * NOTE: this does not deallocate the node (as it may be stack allocated)
 *
 * @param   node      the node to be deleted
 *
 * @return  0   success
 */
int st_node_delete(st_node_t *node) {
    node->st_id = 0;
    node->st_start = 0;
    node->st_end = 0;

    memset(node->st_children, 0,
           DNA_SIGMA_SIZE * sizeof(st_node_t *)); /* NOLINT */
    node->st_slink = NULL;

    return bm_delete(&node->st_text_ids);
}

/**
 * Add a text to the node
 * This basically adds its id as active
 *
 * @param   node            the node where we want the text to
 * @param   text_id         the id of the text: optional with default value -1
 */
void st_node_add_text(st_node_t *node, int text_id) {
    if (text_id != -1) {
        bm_set(&node->st_text_ids, (size_t)text_id);
    }
}

/**
 * Get the size of the edge of this node
 *
 * @param   node            the node where we want the text to
 * @return  size of the edge
 */
size_t st_node_edge_size(st_node_t const *node, size_t default_end) {
    size_t end = node->st_end != SIZE_MAX ? node->st_end : default_end;
    return end + 1 - node->st_start;
}

/**
 * Get the size of the label of this node
 * NOTE: a label size never counts the terminator
 *
 * @param   node            the node where we want the text to
 * @return  size of the label
 */
size_t st_node_label_size(st_node_t const *node) {
    return (st_node_is_leaf(node)) ? node->st_end - node->st_start
                                   : node->st_end + 1 - node->st_start;
}

/**
 * Get the number of active texts in this node
 *
 * @return  number of active texts in this node
 */
size_t st_node_n_texts(st_node_t const *node) {
    return bm_popcnt(&node->st_text_ids);
}

/**
 * Whether a node is a leaf or not
 *
 * @return  true  iff node is a leaf
 */
bool st_node_is_leaf(st_node_t const *node) {
    size_t idx = 0;
    for (idx = 0; idx < DNA_SIGMA_SIZE; ++idx) {
        if (node->st_children[idx] != NULL) {
            return false;
        }
    }

    return true;
}

/* When performing a dfs, the visit will take a node and
 * some argument.
 *
 * This is general enough for some cases, but for others it is somewhat lacking.
 * In these cases, the DFS is re-implemented (it is also not that difficult)
 */
typedef void dfs_visitor_t(st_node_t *, void *);

/**
 * Depth-first-search on a tree node with pre-order visitation
 *
 * @param   node    node to start the DFS on
 * @param   visit   visit function
 * @param   args    arguments for the visitor
 *
 */
void dfs(st_node_t *node, dfs_visitor_t visit, void *args) {
    dna_t idx = 0;
    visit(node, args);

    for (idx = 0; idx < DNA_SIGMA_SIZE; ++idx) {
        if (node->st_children[idx] != NULL) {
            dfs(node->st_children[idx], visit, args);
        }
    }
}

/**
 * Depth-first-search on a tree node with post-order visitation
 *
 * @param   node    node to start the DFS on
 * @param   visit   visit function
 * @param   args    arguments for the visitor
 *
 */
void dfs_post_order(st_node_t *node, dfs_visitor_t visit, void *args) {
    dna_t idx = 0;
    for (idx = 0; idx < DNA_SIGMA_SIZE; ++idx) {
        if (node->st_children[idx] != NULL) {
            dfs_post_order(node->st_children[idx], visit, args);
        }
    }
    visit(node, args);
}

/***
 * The generalized suffix tree
 */
typedef struct st_tree_t {
    /* root of the tree */
    st_node_t *st_root;

    /* texts */
    text_t *st_texts;
    size_t st_n_texts;
} st_tree_t;

/*
 * When creating a suffix tree, there is a lot of state which needs to be passed
 * around This structure aggregates that state, and as such is private to the
 * tree constructor
 */
typedef struct st_builder_t {
    /* leaves corresponding to the current text */
    st_node_t **b_text_leaves;
    size_t b_text_leaves_size;

    /* a node that requires a suffix link */
    st_node_t *b_need_slink;

    /* how many characters are left to process */
    size_t b_remainder;

    /* node being actively worked on */
    st_node_t *b_active_node;

    /* length of the edge being added to */
    size_t b_active_length;

    /* last character added */
    dna_t b_active_edge;

    /* index of the last character added in its string */
    size_t b_active_edge_idx;

    /* default end for an unterminated label */
    size_t b_default_end;

    /* are we in a terminal character? */
    bool b_terminal;
} st_builder_t;

/***
 * Public API
 * */
int st_create(st_tree_t *tree, text_t *text_v, size_t text_s);
int st_delete(st_tree_t *tree);
#ifndef NDEBUG
int st_print(st_tree_t const *tree, FILE *stream);
int st_dot(st_tree_t const *tree, FILE *stream);
#endif

/***
 * Private API
 *
 * auxiliar functions to st_create
 */
int st_add_text(st_tree_t *tree, st_builder_t *builder, text_t const *t);
int st_add_char(st_tree_t *tree, st_builder_t *builder, text_t const *text,
                dna_t ch, size_t phase);
bool st_builder_descend(st_builder_t *builder, text_t const *text,
                        st_node_t *node);
void st_builder_add_suffix_link(st_builder_t *builder, st_node_t *node);
st_node_t *st_add_leaf(st_tree_t *tree, st_builder_t *builder, int text_id,
                       size_t phase);
void st_correct_ids(st_tree_t *tree);

/**
 * Create a new suffix tree, from a set of texts.
 * The suffix tree gains ownership over the texts, and `delete`s them on cleanup
 * (but does not `free` them)
 *
 * @param   tree      the tree to be created
 * @param   text_v    vector of texts
 * @param   text_s    size of vector of texts
 *
 * @return  0   success
 *         -1   failure
 */
int st_create(st_tree_t *tree, text_t *text_v, size_t text_s) {
    st_builder_t builder;
    size_t i = 0;

    tree->st_texts = text_v;
    tree->st_n_texts = text_s;
    tree->st_root = xmalloc(sizeof(st_node_t));
    if (st_node_create(tree->st_root, 0 /* start */,
                       SIZE_MAX /* end: default value */, -1 /* text id */,
                       tree->st_n_texts) != 0) {
        return -1;
    }

    memset(&builder, 0, sizeof(builder));
    builder.b_active_node = tree->st_root;
    builder.b_active_edge = DNA_EPSILON;

    for (i = 0; i < text_s; ++i) {
        if (st_add_text(tree, &builder, &text_v[i]) != 0) {
            free(builder.b_text_leaves);
            st_delete(tree);
            return -1;
        }
    }

    free(builder.b_text_leaves);
    st_correct_ids(tree);
    return 0;
}

/**
 * Visitor for the delete function
 * This simply guarantees that each node is deleted and freed
 *
 * @param   node    node to be deleted
 */
void st_deleter_visitor(st_node_t *node, void *_args) {
    (void)_args; /* ignore arguments */
    st_node_delete(node);
    free(node);
}

/**
 * Delete a suffix tree
 * The suffix tree gain ownership over the texts, and `delete`s them on cleanup
 *
 * @param   tree      the tree to be created
 *
 * @return  0   success
 *         -1   failure
 */
int st_delete(st_tree_t *tree) {
    size_t idx = 0;
    dfs_post_order(tree->st_root, st_deleter_visitor, NULL);

    for (idx = 0; idx < tree->st_n_texts; ++idx) {
        text_delete(&tree->st_texts[idx]);
    }

    return 0;
}

#ifndef NDEBUG
/**
 * Arguments for the print function
 */
struct print_args {
    /* stream to print to */
    FILE *stream;

    /* vector of texts */
    text_t *texts;
    size_t n_texts;
};

/**
 * Visitor for the print function
 *
 * @param   node    node to print
 * @param   args    print_args
 *
 */
void print_visitor(st_node_t *node, void *args) {
    size_t idx = 0;
    struct print_args *p_args = (struct print_args *)args;

    fprintf(p_args->stream, "[%p | %2d] \"", (void *)node, node->st_id);
    if (node->st_id < 0) {
        fprintf(p_args->stream, "<root>\"");
    } else {
        for (idx = node->st_start;
             idx <= node->st_end && idx < p_args->texts[node->st_id].t_size;
             ++idx) {
            fputc(dna_to_char(p_args->texts[node->st_id].t_text[idx]),
                  p_args->stream);
        }
        fprintf(p_args->stream, "\"\ts: %4zu | e: %4zu", node->st_start,
                node->st_end);
    }

    /*
    fputs("\" => [", p_args->stream);
    for (idx = 0; idx < DNA_SIGMA_SIZE; ++idx) {
        if (node->st_children[idx] != NULL) {
            fputc(dna_to_char((int)idx), p_args->stream);
        }
    }

    fputc(']', p_args->stream);

    if (node->st_slink) {
        fprintf(p_args->stream, " ~~~> %p", (void *)node->st_slink);
    }
    */
    fputc('\n', p_args->stream);
}

/**
 * Print a tree
 *
 * @param   tree      the tree to be created
 * @param   stream    stream to direct output to
 *
 * @return  0   success
 */
int st_print(st_tree_t const *tree, FILE *stream) {
    struct print_args args;
    args.stream = stream;
    args.texts = tree->st_texts;
    args.n_texts = tree->st_n_texts;

    dfs(tree->st_root, print_visitor, (void *)&args);

    return 0;
}

void dot_visitor(st_node_t *node, void *args) {
    size_t idx = 0;
    size_t text_idx = 0;
    size_t count = 0;
    st_node_t *child = NULL;
    struct print_args *p_args = (struct print_args *)args;

    for (idx = 0; idx < DNA_SIGMA_SIZE; ++idx) {
        if (node->st_children[idx] != NULL) {
            child = node->st_children[idx];
            count += 1;
            fprintf(p_args->stream, "\t%zu -> %zu [label=\"", node->st_n_id,
                    child->st_n_id);
            for (text_idx = child->st_start;
                 text_idx <= child->st_end &&
                 text_idx < p_args->texts[child->st_id].t_size;
                 ++text_idx) {
                fputc(dna_to_char(p_args->texts[child->st_id].t_text[text_idx]),
                      p_args->stream);
            }
            fputs("\"];\n", p_args->stream);
        }
    }

    if (node->st_slink != NULL) {
        fprintf(p_args->stream, "\t%zu -> %zu [style=dotted];\n", node->st_n_id,
                node->st_slink->st_n_id);
    }

    if (count == 0) {
        for (idx = 0; idx < p_args->n_texts; ++idx) {
            if (bm_get(&node->st_text_ids, idx)) {
                fprintf(p_args->stream, "\t%zu [label=\"%zu\"] [shape=box];\n",
                        node->st_n_id * 1000 + idx, idx); /* NOLINT */
                fprintf(p_args->stream, "\t%zu -> %zu [color=red];\n",
                        node->st_n_id, node->st_n_id * 1000 + idx); /* NOLINT */
            }
        }
    }
}

/**
 * Print a tree in the DOT format
 *
 * @param   tree      the tree to be created
 * @param   stream    stream to direct output to
 *
 * @return  0   success
 */
int st_dot(st_tree_t const *tree, FILE *stream) {
    struct print_args args;
    args.stream = stream;
    args.texts = tree->st_texts;
    args.n_texts = tree->st_n_texts;

    fputs("digraph python_graph {\n", stream);
    dfs(tree->st_root, dot_visitor, (void *)&args);
    fputs("}\n", stream);

    return 0;
}
#endif

/**
 * Add a leaf to the tree
 *
 * @param   tree    tree where we are adding the node
 * @param   builder state of the building process
 * @param   text_id id of the text creating this node
 * @param   phase   phase of processing this text (equiv to start index)
 *
 * @return  new leaf (NULL on error)
 */
st_node_t *st_add_leaf(st_tree_t *const tree, st_builder_t *builder,
                       int text_id, size_t phase) {
    st_node_t *leaf = xmalloc(sizeof(st_node_t));
    if (st_node_create(leaf, phase /* start */,
                       SIZE_MAX /* end: default value */, text_id,
                       tree->st_n_texts) != 0) {
        return NULL;
    }

    builder->b_text_leaves[builder->b_text_leaves_size++] = leaf;

    return leaf;
}

/**
 * TODO: remove this function
 */
void st_correct_visitor(st_node_t *node, void *_args) {
    (void)_args;
    dna_t i = 0;

    for (i = 0; i < DNA_SIGMA_SIZE; ++i) {
        if (node->st_children[i] != NULL) {
            bm_or(&node->st_text_ids, &node->st_children[i]->st_text_ids);
        }
    }
}

/**
 * Correct the ids of a tree
 * TODO: remove this function, making sure this is done inside the tree
 * construction
 *
 * @param   tree      the tree to be created
 */
void st_correct_ids(st_tree_t *tree) {
    dfs_post_order(tree->st_root, st_correct_visitor, NULL);
}

/**
 * Add a new text to the suffix tree
 * Note that the terminator is the additive symmetric of the id of the string
 *
 * @param   tree      the tree to be created
 * @param   text      the text to be added
 * @param   text_id   the id of the text
 *
 * @return  0   success
 *         -1   failure
 */
int st_add_text(st_tree_t *const tree, st_builder_t *builder, text_t const *t) {
    size_t i = 0;
    builder->b_terminal = false;

    builder->b_text_leaves =
        xrealloc(builder->b_text_leaves, t->t_size * sizeof(st_node_t **));
    builder->b_text_leaves_size = 0;

    for (i = 0; i < t->t_size; ++i) {
        if (st_add_char(tree, builder, t, t->t_text[i], i) != 0) {
            return -1;
        }
    }

    for (i = 0; i < builder->b_text_leaves_size; ++i) {
        /* Since the end is inclusive, we want t_size - 1 */
        builder->b_text_leaves[i]->st_end = t->t_size - 1;
    }

    builder->b_text_leaves_size = 0;
    return 0;
}

/**
 * Add a new character to the suffix tree
 *
 * @param   tree      the tree to be created
 * @param   ch        the character to be added
 *
 * @return  0   success
 *         -1   failure
 */
int st_add_char(st_tree_t *const tree, st_builder_t *builder,
                text_t const *text, dna_t ch, size_t phase) {
    st_node_t *next_node = NULL;
    st_node_t *split_node = NULL;
    st_node_t *leaf = NULL;

    builder->b_default_end = phase;
    builder->b_need_slink = NULL;
    builder->b_remainder += 1;

    while (builder->b_remainder > 0) {
        if (builder->b_active_length == 0) {
            builder->b_active_edge_idx = phase;
            builder->b_active_edge = text->t_text[builder->b_active_edge_idx];
        }

        next_node = builder->b_active_node->st_children[builder->b_active_edge];
        if (next_node != NULL) {
            if (st_builder_descend(builder, text, next_node)) {
                continue;
            }

            if (tree->st_texts[next_node->st_id] /* NOLINT */
                    .t_text[next_node->st_start + builder->b_active_length] ==
                ch) {
                /* there is a match */
                if (ch == DNA_TERM) { /* a terminal character */
                    st_node_add_text(next_node, text->t_id);
                    if (!builder->b_terminal) {
                        st_builder_add_suffix_link(builder,
                                                   builder->b_active_node);
                        builder->b_terminal = true;
                    }
                } else { /* we are just matching with the edge label */
                    builder->b_active_length += 1;
                    st_builder_add_suffix_link(builder, builder->b_active_node);
                    break;
                }
            } else {
                /* this does not match, we need to break this up
                 *
                 * the split node is still associated with the previous
                 * node
                 */
                split_node = xmalloc(sizeof(st_node_t));
                if (st_node_create(
                        split_node, next_node->st_start,
                        next_node->st_start + builder->b_active_length - 1,
                        next_node->st_id, tree->st_n_texts) != 0) {
                    return -1;
                }

                builder->b_active_node->st_children[builder->b_active_edge] =
                    split_node;
                leaf = st_add_leaf(tree, builder, text->t_id, phase);
                if (leaf == NULL) {
                    return -1;
                }

                split_node->st_children[ch] = leaf;

                next_node->st_start += builder->b_active_length;
                split_node->st_children[tree->st_texts[next_node->st_id]
                                            .t_text[next_node->st_start]] =
                    next_node;
                st_builder_add_suffix_link(builder, split_node);
            }
        } else {
            leaf = st_add_leaf(tree, builder, text->t_id, phase);
            if (leaf == NULL) {
                return -1;
            }

            builder->b_active_node->st_children[builder->b_active_edge] = leaf;
            st_builder_add_suffix_link(builder, builder->b_active_node);
        }

        if (builder->b_active_node == tree->st_root &&
            builder->b_active_length > 0) {
            builder->b_active_edge_idx += 1;
            builder->b_active_edge = text->t_text[builder->b_active_edge_idx];
            builder->b_active_length -= 1;
        } else if (builder->b_active_node != tree->st_root) {
            builder->b_active_node = builder->b_active_node->st_slink;
        }

        builder->b_remainder -= 1;
    }

    return 0;
}

/**
 * Add a suffix link to the tree.
 *
 * @param   builder builder to add the (stored) suffix link to
 * @param   node    suffix link pointer
 */
void st_builder_add_suffix_link(st_builder_t *const builder, st_node_t *node) {
    if (builder->b_need_slink != NULL) {
        builder->b_need_slink->st_slink = node;
    }

    builder->b_need_slink = node;
}

/**
 * Descend in the subtree
 *
 * @param   builder the builder
 * @param   text    text being added
 * @param   node    current node
 *
 * @return  whether it did descend or not
 */
bool st_builder_descend(st_builder_t *builder, text_t const *text,
                        st_node_t *node) {
    size_t edge_length = st_node_edge_size(node, builder->b_default_end);
    if (builder->b_active_length >= edge_length) {
        builder->b_active_length -= edge_length;
        builder->b_active_edge_idx += edge_length;
        builder->b_active_edge = text->t_text[builder->b_active_edge_idx];
        builder->b_active_node = node;

        return true;
    }

    return false;
}

/***
 * Vector of nodes
 */
typedef struct st_node_vec_t {
    st_node_t **stv_ptr;
    size_t stv_size;
    size_t stv_cap;
} st_node_vec_t;

/**
 * Create a node vector.
 * A node vector begins empty.
 *
 * @param   vec the node vector
 *
 * @return  0   success
 */
int st_node_vec_create(st_node_vec_t *vec) {
    memset(vec, 0, sizeof(st_node_vec_t));
    return 0;
}

/**
 * Delete a node vector
 *
 * @param   vec the node vector
 *
 * @return  0   success
 */
int st_node_vec_delete(st_node_vec_t *vec) {
    if (vec->stv_ptr != NULL) {
        free(vec->stv_ptr);
    }
    memset(vec, 0, sizeof(st_node_vec_t));
    return 0;
}

/**
 * Push a node into the vector. May reallocate
 *
 * @param   vec     the node vector
 * @param   node    the node
 *
 * @return  0   success
 */
int st_node_vec_push(st_node_vec_t *vec, st_node_t *node) {
    if (vec->stv_cap == vec->stv_size) {
        vec->stv_cap = (vec->stv_cap > 0) ? vec->stv_cap * 2 : DEFAULT_VEC_SIZE;

        /* Cool tidbit: realloc(NULL, N) <=> malloc(N) */
        vec->stv_ptr =
            xrealloc(vec->stv_ptr, vec->stv_cap * sizeof(st_node_t *));
    }

    vec->stv_ptr[vec->stv_size] = node;
    vec->stv_size += 1;

    return 0;
}

/**
 * Compute the sizes of the Least-common-substrings of k strings (with k from 2
 * to n_texts) Performs a DFS on the generalized suffix tree
 *
 * @param   node    node to start the DFS on
 * @param   lcs     [IN/OUT] - array to place the incremental results of the LCS
 * sizes
 * @param   sdepth  current string depth
 * @param   n_texts number of texts in the suffix tree
 *
 * @return  0       success
 */
int st_node_lcs(st_node_t *node, size_t *lcs, size_t sdepth, size_t n_texts) {
    int ret = 0;
    dna_t idx = 0;
    size_t num_texts = st_node_n_texts(node);

    if (num_texts < 2) {
        /* no need to go on */
        return 0;
    }

    sdepth += st_node_label_size(node);
    for (; num_texts >= 2; --num_texts) {
        if (lcs[num_texts - 2] < sdepth) {
            lcs[num_texts - 2] = sdepth;
        }
    }

    for (idx = 0; idx < DNA_SIGMA_SIZE; ++idx) {
        if (node->st_children[idx] != NULL) {
            ret = st_node_lcs(node->st_children[idx], lcs, sdepth, n_texts);
            if (ret != 0) {
                return ret;
            }
        }
    }

    return 0;
}

/* Compute the sizes of the longest least common substring among k of the
 * strings in the subtrees k \in [2,n_texts]
 *
 * This is computed by performing a DFS in the tree.
 * The DFS will maintain 2 types of state:
 *  - vector of maximum size of the lcs between k strings;
 *  - current string depth
 *
 * This will also check how many texts are active in a given path.
 * Safety: lcs is a pointer to a vector with n_texts - 1 elements
 *
 * @param tree  Generalized suffix tree
 *
 * @return 0 success
 */
int st_least_common_substr(st_tree_t *tree, size_t *lcs) {
    int ret = 0;
    size_t idx = 0;
    for (idx = 0; idx < DNA_SIGMA_SIZE; ++idx) {
        if (tree->st_root->st_children[idx] != NULL) {
            ret = st_node_lcs(tree->st_root->st_children[idx], lcs, 0,
                              tree->st_n_texts);
            if (ret != 0) {
                return ret;
            }
        }
    }
    return 0;
}

/**
 * Parse the input into a vector of texts
 *
 * @param   t_size [OUT] - number of texts
 * @return  vector of texts
 */
text_t *parse_input(size_t *t_size) {
    size_t i = 0;
    text_t *texts = NULL;

    int n_matches = fscanf(stdin, "%zu\n", t_size); /* NOLINT */
    assert(n_matches == 1, "failed to get the number of texts");
    texts = xcalloc(*t_size, sizeof(text_t));

    for (i = 0; i < *t_size; i++) {
        size_t size = 0;
        n_matches = fscanf(stdin, "%zu ", &size); /* NOLINT */
        assert(n_matches == 1, "failed to get the length of a test");

        text_create(&texts[i], stdin, size, (int)i);
    }

    return texts;
}

int main() {
    size_t t_size = 0;
    size_t *lcs = NULL;
    size_t idx = 0;

    st_tree_t generalized_tree;
#ifndef NDEBUG
    FILE *stream = NULL;
#endif

    text_t *texts = parse_input(&t_size);
    assert(st_create(&generalized_tree, texts, t_size) == 0,
           "failed to create the tree");

#ifndef NDEBUG
    stream = fopen("c_sol.dot", "we");
    st_dot(&generalized_tree, stream);
    fclose(stream);
#endif

    lcs = xcalloc(generalized_tree.st_n_texts - 1, sizeof(size_t));
    assert(st_least_common_substr(&generalized_tree, lcs) == 0,
           "failed to compute lcs");

    for (idx = 2; idx <= generalized_tree.st_n_texts; ++idx) {
        fprintf(stdout, "%zu ", lcs[idx - 2]);
    }
    fputc('\n', stdout);

    free(lcs);
    st_delete(&generalized_tree);
    free(texts);

    return 0;
}
