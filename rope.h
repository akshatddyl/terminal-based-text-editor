#ifndef ROPE_H
#define ROPE_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LEAF_SIZE 32  // Maximum characters in a leaf node

typedef struct RopeNode {
    int weight;       // Length of left subtree (or length of text if leaf)
    int newlines;     // Number of newlines in left subtree (or in text if leaf)
    int total_length; // Total length of the entire node/subtree
    int total_newlines; // Total newlines of the entire node/subtree
    int depth;        // Depth of the node in the tree
    char *text;       // Text data (only for leaf nodes)
    struct RopeNode *left;
    struct RopeNode *right;
    int is_leaf;
} RopeNode;

// Core functions
RopeNode* rope_create(const char *str);
RopeNode* rope_insert(RopeNode *root, int position, const char *str);
RopeNode* rope_delete(RopeNode *root, int start, int length);
RopeNode* rope_concat(RopeNode *left, RopeNode *right);
void rope_free(RopeNode *root);

// Info functions
int rope_length(const RopeNode *root);
int rope_newlines(const RopeNode *root);
int rope_line_to_pos(const RopeNode *root, int line);
void rope_pos_to_line_col(const RopeNode *root, int pos, int *line, int *col);
char rope_char_at(const RopeNode *root, int position);
char* rope_to_string(const RopeNode *root);

// Iterator for efficient traversal
typedef struct {
    const RopeNode *stack[128];
    int stack_size;
    int leaf_pos; // current position within the leaf text
} RopeIterator;

void rope_iter_init(RopeIterator *iter, const RopeNode *root);
int rope_iter_seek(RopeIterator *iter, const RopeNode *root, int position);
int rope_iter_next(RopeIterator *iter, char *c);

// Rebalancing
RopeNode* rope_rebalance(RopeNode *root);

// Debug: recursive invariant checker. Aborts on first violation.
void validate_rope(RopeNode *node);

#endif
