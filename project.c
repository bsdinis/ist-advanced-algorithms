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

typedef enum { false = 0, true = 1 } bool;

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
#define xcalloc(nmemb, size) _priv_xcalloc__(__FILE__, __LINE__, nmemb, size)
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
 *
 * This is defined as an integer.
 * Negative integers are the terminators.
 */
typedef int dna_t;
const int DNA_A = 0;
const int DNA_C = 1;
const int DNA_T = 2;
const int DNA_G = 3;
#define DNA_SIGMA_SIZE (4)  /* size of the alphabet */

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
    int    t_id;
} text_t;

static int text_create(text_t *text, FILE *stream, size_t size, int id) {
    text->t_id = id;
    text->t_size = size + 1;
    text->t_text = xmalloc((size + 1) * sizeof(dna_t));
#ifdef NDEBUG
    readline_into_buffer(stream, &text->t_text, size);
#else
    text->t_str = xmalloc((size + 1) * sizeof(char));
    readline_into_buffer_str(stream, text->t_str, text->t_text, size);
#endif  /* NDEBUG */

    /* Add the terminator */
    text->t_text[size] = -id;

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
    text->t_id = 0;

    return 0;
}

/* ------------------------------------------------------------------------- */

/* Node of the suffix tree
 *
 * This node has:
 *  - a label in the range [st_start, st_end[ in the st_id string
 *  - a set of children
 *  - an (optional) suffix link
 *  - a set of active texts and corresponding start idx (ie: where the label starts in that text)
 */
typedef struct st_node_t
{
    /* id of the text which created the node
     *
     * NOTE: this is important because the st_start and st_end will refer to this text
     * NOTE: this will be at times optional (with value -1)
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
    struct st_node_t* st_children[DNA_SIGMA_SIZE];

    /* suffix link */
    struct st_node_t* st_slink;

    /* starting indexes on this node.
     * these are *optional* values, with default value SIZE_MAX;
     *
     * NOTE (1): this is a vector allocated with N `size_t`s, where N is the numbero of texts
     * This greatly simplifies the implementation (no need to implement resizable vectors)
     * and makes the access faster (always O(1)). However, it does incurr a higher memory
     * footprint. We believe this is acceptable given that the limiting factor (memory wise)
     * are the texts and the number of nodes in the tree.
     *
     * NOTE (2): no need to store the length, as long as it is indexed by a valid node id, all is well
     */
    size_t * st_start_idx;
} st_node_t;

int     st_node_create(st_node_t * node, size_t start, size_t end, int id, size_t start_idx, size_t n_texts);
int     st_node_delete(st_node_t * node);
void    st_node_add_text(st_node_t * node, int text_id, size_t start_offset);
size_t  st_node_label_size(st_node_t const * node, size_t default_end);

/**
 * Create a new suffix tree node.
 *
 * @param   node      the node to be created
 * @param   start     start index in the text
 * @param   end       end index in the text (one past the end): this field is optional with default value SIZE_MAX
 * @param   id        id of the text which creates the node: this field is optional with default value -1
 * @param   start_idx ????? (default value SIZE_MAX)
 *
 * @return  0   success
 */
int st_node_create(st_node_t * node, size_t start, size_t end, int id, size_t start_idx, size_t n_texts) {
    node->st_id = id;
    node->st_start = start;
    node->st_end = end;

    memset(node->st_children, 0, DNA_SIGMA_SIZE * sizeof(st_node_t *));
    node->st_slink = NULL;

    node->st_start_idx = xcalloc(n_texts, sizeof(size_t));

    st_node_add_text(node, id, start_idx);

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
int st_node_delete(st_node_t * node) {
    node->st_id = 0;
    node->st_start = 0;
    node->st_end = 0;

    memset(node->st_children, 0, DNA_SIGMA_SIZE * sizeof(st_node_t *));
    node->st_slink = NULL;

    free(node->st_start_idx);
    node->st_start_idx = NULL;

    return 0;
}

/**
 * Add a text to the node
 * This basically adds its id as active and its start idx
 *
 * @param   node            the node where we want the text to
 * @param   text_id         the id of the text: optional with default value -1
 * @param   start_offset    where the label starts in the text: optional with default value SIZE_MAX
 */
void st_node_add_text(st_node_t * node, int text_id, size_t start_offset) {
    if (text_id != -1 && start_offset != SIZE_MAX) {
        node->st_start_idx[text_id - 1] = start_offset;
    }
}

/**
 * Get the size of the label of this node
 *
 * @param   node            the node where we want the text to
 * @return  size of the label
 */
size_t  st_node_label_size(st_node_t const * node, size_t default_end) {
    size_t end = node->st_end != 0 ? node->st_end : default_end;
    return end - node->st_start;
}

typedef void dfs_visitor_t(st_node_t *, void *);

void dfs(st_node_t * node, dfs_visitor_t visit, void * args) {
    dna_t idx;
    visit(node, args);

    for (idx = 0; idx < DNA_SIGMA_SIZE; ++idx) {
        if (node->st_children[idx] != NULL) {
            dfs(node->st_children[idx], visit, args);
        }
    }
}

void dfs_postorder(st_node_t * node, dfs_visitor_t visit, void * args) {
    dna_t idx;

    for (idx = 0; idx < DNA_SIGMA_SIZE; ++idx) {
        if (node->st_children[idx] != NULL) {
            dfs(node->st_children[idx], visit, args);
        }
    }
    visit(node, args);
}


/* The suffix tree
 */
typedef struct st_tree_t {
    /* root of the tree */
    st_node_t * st_root;

    /* texts */
    text_t * st_texts;
    size_t   st_n_texts;
} st_tree_t;

/*
 * When creating a suffix tree, there is a lot of state which needs to be passed around
 * This structure aggregates that state, and as such is private to the tree constructor
 */
typedef struct st_builder_t {
    /* leaves corresponding to the current text */
    st_node_t ** b_text_leaves;
    size_t       b_text_leaves_size;

    /* a node that requires a suffix link */
    st_node_t * b_need_slink;

    /* how many characters are left to process */
    size_t b_remainder;

    /* ??? */
    st_node_t * b_active_node;

    /* ??? */
    size_t b_active_length;

    /* ??? */
    dna_t b_active_edge;

    /* ??? */
    size_t b_active_edge_idx;

    /* ??? */
    size_t b_phase;

    /* ??? */
    size_t b_start_idx;

    /* ??? */
    size_t b_default_end;

    /* terminal */
    bool b_terminal;
} st_builder_t;

int st_create(st_tree_t * tree, text_t * text_v, size_t text_s);
int st_delete(st_tree_t * tree);
int st_add_text(st_tree_t * const tree, st_builder_t * builder, text_t const * t);
int st_add_char(st_tree_t * const tree, st_builder_t * builder, text_t const * text, dna_t ch);
bool st_builder_descend(st_builder_t * builder, text_t const * text, st_node_t * node);
void st_builder_add_suffix_link(st_builder_t * const builder, st_node_t * node);
st_node_t * st_add_leaf(st_tree_t * const tree, st_builder_t * builder, int text_id);

/**
 * Create a new suffix tree, from a set of texts.
 * The suffix tree gains ownership over the texts, and `delete`s them on cleanup (but does not `free` them)
 *
 * @param   tree      the tree to be created
 * @param   text_v    vector of texts
 * @param   text_s    size of vector of texts
 *
 * @return  0   success
 *         -1   failure
 */
int st_create(st_tree_t * tree, text_t * text_v, size_t text_s) {
    st_builder_t builder;
    size_t i = 0;

    tree->st_texts = text_v;
    tree->st_n_texts = text_s;
    tree->st_root = xmalloc(sizeof(st_node_t *));
    if (st_node_create(
                tree->st_root,
                0 /* start */,
                SIZE_MAX /* end: default value */,
                -1 /* text id */,
                SIZE_MAX /* start_idx: default value */,
                tree->st_n_texts) != 0) {
        return -1;
    }

    free(builder.b_text_leaves);
    memset(&builder, 0, sizeof(builder));
    builder.b_active_node = tree->st_root;

    for (i = 0; i < text_s; ++i) {
        if (st_add_text(tree, &builder, &text_v[i]) != 0) {
            st_delete(tree);
            return -1;
        }
    }

    return 0;
}

void st_deleter_visitor(st_node_t * node, void *_args) {
    (void)_args;
    st_node_delete(node);
    free(node);
}

/**
 * Delete a suffix tree
 * The suffix tree gain ownership over the texts, and `delete`s them on cleanup
 *
 * @param   tree      the tree to be created
 * @param   text_v    vector of texts
 * @param   text_s    size of vector of texts
 *
 * @return  0   success
 *         -1   failure
 */
int st_delete(st_tree_t * tree) {
    size_t idx = 0;
    dfs(tree->st_root, st_deleter_visitor, NULL);

    for (idx = 0; idx < tree->st_n_texts; ++idx) {
        text_delete(&tree->st_texts[idx]);
    }

    return 0;
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
int st_add_text(st_tree_t * const tree, st_builder_t * builder, text_t const * t) {
    size_t i = 0;
    builder->b_terminal = false;
    builder->b_phase = 0;
    builder->b_start_idx = 0;

    builder->b_text_leaves = xrealloc(builder->b_text_leaves, t->t_size * sizeof(st_node_t **));
    builder->b_text_leaves_size = 0;

    for (i = 0; i < t->t_size; ++i) {
        if (st_add_char(tree, builder, t, t->t_text[i]) != 0) {
            return -1;
        }
    }

    for (i = 0; i < builder->b_text_leaves_size; ++i) {
        builder->b_text_leaves[i]->st_end = t->t_size;
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
int st_add_char(st_tree_t * const tree, st_builder_t * builder, text_t const * text, dna_t ch) {
    st_node_t * next_node = NULL;
    st_node_t * split_node = NULL;
    st_node_t * leaf = NULL;

    builder->b_default_end = builder->b_phase;
    builder->b_need_slink = NULL;
    builder->b_remainder += 1;

    while (builder->b_remainder > 0) {
        if (builder->b_active_length == 0) {
            builder->b_active_edge_idx = builder->b_phase;
            builder->b_active_edge = text->t_text[builder->b_active_edge_idx];
        }

        next_node = builder->b_active_node->st_children[builder->b_active_edge];
        if (next_node != NULL) {
            if (st_builder_descend(builder, text, next_node)) {
                continue;
            }

            if (tree->st_texts[next_node->st_id].t_text[next_node->st_start + builder->b_active_length] == ch) {
                /* there is a match */
                if (ch < 0) { /* a terminal character */
                    st_node_add_text(next_node, text->t_id, builder->b_start_idx);
                    builder->b_start_idx += 1;
                    if (!builder->b_terminal) {
                        st_builder_add_suffix_link(builder, builder->b_active_node);
                        builder->b_terminal = true;
                    }
                } else { /* we are just matching with the edge label */
                    builder->b_active_length += 1;
                    st_builder_add_suffix_link(builder, builder->b_active_node);
                }
            } else {
                /* this does not match, we need to break this up
                 *
                 * the split node is still associated with the previous
                 * node
                 */
                split_node = xmalloc(sizeof(st_node_t));
                if (st_node_create(
                            split_node,
                            next_node->st_start,
                            next_node->st_start + builder->b_active_length - 1,
                            next_node->st_id,
                            SIZE_MAX,
                            tree->st_n_texts) != 0) {
                    return -1;
                }

                builder->b_active_node->st_children[builder->b_active_edge] = split_node;
                leaf = st_add_leaf(tree, builder, text->t_id);
                if (leaf == NULL) {
                    return -1;
                }

                split_node->st_children[ch] = leaf;

                next_node->st_start += builder->b_active_length;
                split_node->st_children[tree->st_texts[next_node->st_id].t_text[next_node->st_start]] = next_node;
                st_builder_add_suffix_link(builder, split_node);
            }
        } else {
            leaf = st_add_leaf(tree, builder, text->t_id);
            if (leaf == NULL) {
                return -1;
            }

            builder->b_active_node->st_children[builder->b_active_edge] = leaf;
            st_builder_add_suffix_link(builder, builder->b_active_node);
        }

        if (builder->b_active_node == tree->st_root && builder->b_active_length > 0) {
            builder->b_active_edge_idx += 1;
            builder->b_active_edge = text->t_text[builder->b_active_edge_idx];
            builder->b_active_length -= 1;
        } else if (builder->b_active_node != tree->st_root) {
            builder->b_active_node = builder->b_active_node->st_slink;
        }

        builder->b_remainder -= 1;
    }

    builder->b_phase += 1;

    return 0;
}

void st_builder_add_suffix_link(st_builder_t * const builder, st_node_t * node) {
    if (builder->b_need_slink != NULL) {
        builder->b_need_slink->st_slink = node;
    }

    builder->b_need_slink = node;
}

bool st_builder_descend(st_builder_t * builder, text_t const * text, st_node_t * node) {
    size_t edge_length = st_node_label_size(node, builder->b_default_end);
    if (builder->b_active_length >= edge_length) {
        builder->b_active_length -= edge_length;
        builder->b_active_edge_idx += edge_length;
        builder->b_active_edge = text->t_text[builder->b_active_edge_idx];
        builder->b_active_node = node;

        return true;
    }

    return false;
}

typedef struct st_node_vec_t {
    st_node_t ** stv_ptr;
    size_t stv_size;
    size_t stv_cap;
} st_node_vec_t;

int st_node_vec_create(st_node_vec_t * vec) {
    memset(vec, 0, sizeof(st_node_vec_t));
    return 0;
}

int st_node_vec_delete(st_node_vec_t * vec) {
    if (vec->stv_ptr != NULL) {
        free(vec->stv_ptr);
    }
    memset(vec, 0, sizeof(st_node_vec_t));
    return 0;
}

int st_node_vec_push(st_node_vec_t * vec, st_node_t * node) {
    if (vec->stv_cap == vec->stv_size) {
        vec->stv_cap = (vec->stv_cap > 0) ? vec->stv_cap * 2 : 16;

        /* Cool tidbit: realloc(NULL, N) <=> malloc(N) */
        vec->stv_ptr = xrealloc(vec->stv_ptr, vec->stv_cap * sizeof(st_node_t *));
    }

    vec->stv_ptr[vec->stv_size] = node;
    vec->stv_size += 1;

    return 0;
}

struct lcs_visitor_args {
    st_node_vec_t * L;
    size_t n_texts;
};

/* This visitor adds all the strings in the leaves */
void lcs_visitor(st_node_t * node, void * args) {
    struct lcs_visitor_args * lcs_args = (struct lcs_visitor_args *)args;
    st_node_vec_t * L = lcs_args->L;
    size_t n_texts = lcs_args->n_texts;
    bool is_leaf = true;
    dna_t idx = 0;
    size_t i = 0;
    for (idx = 0; is_leaf && idx < DNA_SIGMA_SIZE; ++idx) {
        if (node->st_children[idx] != NULL) {
            is_leaf = false;
        }
    }

    for (i = 0; i < n_texts; ++i) {
        if (node->st_start_idx[i] != SIZE_MAX) {
            assert(st_node_vec_push(&L[i], node) == 0, "failed to push to node vec");
        }
    }
}

/* Compute the size of the least common substring
 *
 * @param tree  Generalized suffix tree
 *
 * @return size of the least common substring
 */
size_t st_least_common_substr_size(st_tree_t * tree) {
    size_t idx = 0;
    struct lcs_visitor_args args;
    st_node_vec_t * L = xcalloc(tree->st_n_texts, sizeof(st_node_vec_t));
    for (idx = 0; idx < tree->st_n_texts; ++idx) {
        assert(st_node_vec_create(&L[idx]) == 0, "failed to create node vector");
    }

    args.L = L;
    args.n_texts = tree->st_n_texts;
    dfs(tree->st_root, lcs_visitor, (void *)&args);

    return 0;
}

st_node_t * st_add_leaf(st_tree_t * const tree, st_builder_t * builder, int text_id) {
    st_node_t * leaf = xmalloc(sizeof(st_node_t));
    if (st_node_create(
                leaf,
                builder->b_phase /* start */,
                SIZE_MAX /* end: default value */,
                text_id,
                builder->b_start_idx /* start_idx: default value */,
                tree->st_n_texts) != 0) {
        return NULL;
    }

    builder->b_text_leaves[builder->b_text_leaves_size++] = leaf;

    return leaf;
}

/* ------------------------------------------------------------------------- */

static text_t * parse_input(size_t * t_size, size_t * max_size) {
    size_t i = 0;
    text_t * texts = NULL;

    int n_matches = scanf("%zu\n", t_size);
    assert(n_matches == 1, "failed to get the number of texts");
    texts = xcalloc(*t_size, sizeof(text_t));

    for (i = 0; i < *t_size; i++) {
        size_t size;
        n_matches = scanf("%zu ", &size);
        assert(n_matches == 1, "failed to get the length of a test");

        if (size > *max_size) {
            *max_size = size;
        }

        /* IDs need to start at 1.
         * Otherwise the terminator -0 would collide with DNA_A
         */
        text_create(&texts[i], stdin, size, ((int)i) + 1);
    }

    return texts;
}


int main() {
    size_t t_size = 0;
    size_t max_text_size = 0;
    text_t * texts = parse_input(&t_size, &max_text_size);
    st_tree_t generalized_tree;
    st_create(&generalized_tree, texts, t_size);

    st_delete(&generalized_tree);
    free(texts);

    return 0;
}
