#ifndef ROPE_H
#define ROPE_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LEAF_SIZE 32  // Maximum characters in a leaf node

typedef struct RopeNode {
    int weight;              // Number of characters in left subtree (or in this leaf)
    char *text;              // Text data (only for leaf nodes)
    struct RopeNode *left;
    struct RopeNode *right;
    int is_leaf;
} RopeNode;

// Function prototypes
RopeNode* rope_create(const char *str);
RopeNode* rope_insert(RopeNode *root, int position, const char *str);
RopeNode* rope_delete(RopeNode *root, int start, int length);
RopeNode* rope_concat(RopeNode *left, RopeNode *right);
char* rope_to_string(RopeNode *root);
int rope_length(RopeNode *root);
void rope_free(RopeNode *root);
char rope_char_at(RopeNode *root, int position);

#endif
