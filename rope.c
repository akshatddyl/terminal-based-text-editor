#include "rope.h"
#include <assert.h>

// Count newlines in a string
static int count_newlines(const char *str) {
    int count = 0;
    while (*str) {
        if (*str == '\n') count++;
        str++;
    }
    return count;
}

// Create a leaf node
static RopeNode* create_leaf(const char *str) {
    RopeNode *node = (RopeNode*)malloc(sizeof(RopeNode));
    node->is_leaf = 1;
    node->weight = strlen(str);
    node->newlines = count_newlines(str);
    node->total_length = node->weight;
    node->total_newlines = node->newlines;
    node->depth = 1;
    node->text = (char*)malloc(node->weight + 1);
    strcpy(node->text, str);
    node->left = NULL;
    node->right = NULL;
    return node;
}

// Create an internal node in O(1) time
static RopeNode* create_internal(RopeNode *left, RopeNode *right) {
    RopeNode *node = (RopeNode*)malloc(sizeof(RopeNode));
    node->is_leaf = 0;
    node->text = NULL;
    node->left = left;
    node->right = right;
    
    // O(1) property calculations
    node->weight = left ? left->total_length : 0;
    node->newlines = left ? left->total_newlines : 0;
    
    int right_len = right ? right->total_length : 0;
    int right_nl = right ? right->total_newlines : 0;
    
    node->total_length = node->weight + right_len;
    node->total_newlines = node->newlines + right_nl;
    
    int left_depth = left ? left->depth : 0;
    int right_depth = right ? right->depth : 0;
    node->depth = 1 + (left_depth > right_depth ? left_depth : right_depth);
    
    return node;
}

RopeNode* rope_create(const char *str) {
    if (str == NULL || strlen(str) == 0) {
        return create_leaf("");
    }
    return create_leaf(str);
}

// O(1) Length retrieval
int rope_length(const RopeNode *root) {
    if (root == NULL) return 0;
    return root->total_length;
}

// O(1) Newlines retrieval
int rope_newlines(const RopeNode *root) {
    if (root == NULL) return 0;
    return root->total_newlines;
}

int rope_line_to_pos(const RopeNode *root, int line) {
    if (root == NULL || line <= 0) return 0;
    if (root->is_leaf) {
        int pos = 0;
        int current_line = 0;
        while (current_line < line && pos < root->weight) {
            if (root->text[pos] == '\n') current_line++;
            pos++;
        }
        return pos;
    }
    
    if (line <= root->newlines) {
        return rope_line_to_pos(root->left, line);
    } else {
        return root->weight + rope_line_to_pos(root->right, line - root->newlines);
    }
}

void rope_pos_to_line_col(const RopeNode *root, int pos, int *line, int *col) {
    *line = 0;
    *col = 0;
    if (root == NULL || pos <= 0) return;
    
    if (root->is_leaf) {
        int limit = (pos < root->weight) ? pos : root->weight;
        for (int i = 0; i < limit; i++) {
            if (root->text[i] == '\n') {
                (*line)++;
                *col = 0;
            } else {
                (*col)++;
            }
        }
        return;
    }
    
    if (pos <= root->weight) {
        rope_pos_to_line_col(root->left, pos, line, col);
    } else {
        int l_line, l_col;
        rope_pos_to_line_col(root->left, root->weight, &l_line, &l_col);
        
        int r_line, r_col;
        rope_pos_to_line_col(root->right, pos - root->weight, &r_line, &r_col);
        
        *line = l_line + r_line;
        if (r_line > 0) {
            *col = r_col;
        } else {
            *col = l_col + r_col;
        }
    }
}

char rope_char_at(const RopeNode *root, int position) {
    if (root == NULL) return '\0';
    if (root->is_leaf) {
        if (position < 0 || position >= root->weight) return '\0';
        return root->text[position];
    }
    if (position < root->weight) {
        return rope_char_at(root->left, position);
    } else {
        return rope_char_at(root->right, position - root->weight);
    }
}

// Rebalancing logic
static void collect_leaves(RopeNode *node, RopeNode ***leaves, int *count, int *capacity) {
    if (!node) return;
    if (node->is_leaf) {
        if (*count >= *capacity) {
            *capacity *= 2;
            *leaves = (RopeNode**)realloc(*leaves, *capacity * sizeof(RopeNode*));
        }
        (*leaves)[*count] = node;
        (*count)++;
    } else {
        collect_leaves(node->left, leaves, count, capacity);
        collect_leaves(node->right, leaves, count, capacity);
        free(node); // Free internal node as we rebuild
    }
}

static RopeNode* build_balanced(RopeNode **leaves, int start, int end) {
    if (start > end) return NULL;
    if (start == end) return leaves[start];
    
    int mid = start + (end - start) / 2;
    RopeNode *left = build_balanced(leaves, start, mid);
    RopeNode *right = build_balanced(leaves, mid + 1, end);
    return create_internal(left, right);
}

RopeNode* rope_rebalance(RopeNode *root) {
    if (!root) return NULL;
    int capacity = 128;
    RopeNode **leaves = (RopeNode**)malloc(capacity * sizeof(RopeNode*));
    int count = 0;
    
    collect_leaves(root, &leaves, &count, &capacity);
    
    RopeNode *balanced = build_balanced(leaves, 0, count - 1);
    free(leaves);
    return balanced;
}

// Split rope at position
static void rope_split(RopeNode *root, int position, RopeNode **left_out, RopeNode **right_out) {
    if (root == NULL) {
        *left_out = NULL;
        *right_out = NULL;
        return;
    }
    if (root->is_leaf) {
        if (position <= 0) {
            *left_out = create_leaf("");
            *right_out = root;
        } else if (position >= root->weight) {
            *left_out = root;
            *right_out = create_leaf("");
        } else {
            char *left_text = (char*)malloc(position + 1);
            char *right_text = (char*)malloc(root->weight - position + 1);
            strncpy(left_text, root->text, position);
            left_text[position] = '\0';
            strcpy(right_text, root->text + position);
            
            *left_out = create_leaf(left_text);
            *right_out = create_leaf(right_text);
            
            free(left_text);
            free(right_text);
            rope_free(root);
        }
        return;
    }
    
    if (position < root->weight) {
        RopeNode *ll, *lr;
        rope_split(root->left, position, &ll, &lr);
        *left_out = ll;
        *right_out = rope_concat(lr, root->right);
        free(root);
    } else if (position > root->weight) {
        RopeNode *rl, *rr;
        rope_split(root->right, position - root->weight, &rl, &rr);
        *left_out = rope_concat(root->left, rl);
        *right_out = rr;
        free(root);
    } else {
        *left_out = root->left;
        *right_out = root->right;
        free(root);
    }
}

RopeNode* rope_concat(RopeNode *left, RopeNode *right) {
    if (left == NULL) return right;
    if (right == NULL) return left;
    
    // Merge small leaves
    if (left->is_leaf && right->is_leaf && left->weight + right->weight <= LEAF_SIZE) {
        char *merged = (char*)malloc(left->weight + right->weight + 1);
        strcpy(merged, left->text);
        strcat(merged, right->text);
        RopeNode *result = create_leaf(merged);
        free(merged);
        rope_free(left);
        rope_free(right);
        return result;
    }
    return create_internal(left, right);
}

RopeNode* rope_insert(RopeNode *root, int position, const char *str) {
    if (str == NULL || strlen(str) == 0) return root;
    assert(position >= 0 && position <= rope_length(root));
    
    RopeNode *left, *right;
    rope_split(root, position, &left, &right);
    RopeNode *new_node = create_leaf(str);
    RopeNode *temp = rope_concat(left, new_node);
    RopeNode *result = rope_concat(temp, right);
    
    // Selectively rebalance to avoid massive O(N log N) overhead on keystrokes
    if (result && result->depth > 64) {
        return rope_rebalance(result);
    }
    return result;
}

RopeNode* rope_delete(RopeNode *root, int start, int length) {
    if (length <= 0) return root;
    assert(start >= 0 && start + length <= rope_length(root));
    
    RopeNode *left, *mid_right;
    rope_split(root, start, &left, &mid_right);
    RopeNode *mid, *right;
    rope_split(mid_right, length, &mid, &right);
    rope_free(mid);
    
    RopeNode *result = rope_concat(left, right);
    if (result && result->depth > 64) {
        return rope_rebalance(result);
    }
    return result;
}

void rope_free(RopeNode *root) {
    if (root == NULL) return;
    if (root->is_leaf) {
        free(root->text);
    } else {
        rope_free(root->left);
        rope_free(root->right);
    }
    free(root);
}

// String conversion using Iterator
char* rope_to_string(const RopeNode *root) {
    int len = rope_length(root);
    char *result = (char*)malloc(len + 1);
    if (len == 0) {
        result[0] = '\0';
        return result;
    }
    
    RopeIterator iter;
    rope_iter_init(&iter, root);
    char c;
    int idx = 0;
    while (rope_iter_next(&iter, &c) && idx < len) {
        result[idx++] = c;
    }
    result[idx] = '\0';
    return result;
}

// Iterator implementation
static void push_left(RopeIterator *iter, const RopeNode *node) {
    while (node) {
        iter->stack[iter->stack_size++] = node;
        node = node->left;
    }
}

void rope_iter_init(RopeIterator *iter, const RopeNode *root) {
    iter->stack_size = 0;
    iter->leaf_pos = 0;
    push_left(iter, root);
}

int rope_iter_seek(RopeIterator *iter, const RopeNode *root, int position) {
    iter->stack_size = 0;
    iter->leaf_pos = 0;
    const RopeNode *current = root;
    int current_pos = 0;
    
    while (current) {
        if (current->is_leaf) {
            // Always push the target leaf
            iter->stack[iter->stack_size++] = current;
            if (position - current_pos <= current->weight) {
                iter->leaf_pos = position - current_pos;
                return 1; // Success
            }
            return 0; // Out of bounds
        }
        
        if (position < current_pos + current->weight) {
            // Going LEFT: push this internal node onto the stack.
            // rope_iter_next will later pop it and traverse its right child.
            iter->stack[iter->stack_size++] = current;
            current = current->left;
        } else {
            // Going RIGHT: do NOT push this internal node.
            // We are skipping its entire left subtree. If we pushed it,
            // rope_iter_next would pop it and re-push node->right,
            // causing double-traversal and corrupted output.
            current_pos += current->weight;
            current = current->right;
        }
    }
    return 0;
}

int rope_iter_next(RopeIterator *iter, char *c) {
    while (iter->stack_size > 0) {
        const RopeNode *node = iter->stack[iter->stack_size - 1];
        
        if (node->is_leaf) {
            if (iter->leaf_pos < node->weight) {
                *c = node->text[iter->leaf_pos++];
                return 1;
            } else {
                // Reached end of leaf, pop it
                iter->stack_size--;
                iter->leaf_pos = 0;
            }
        } else {
            // Internal node: we've already processed its left child (because it was popped)
            // Now pop the internal node and push its right child
            iter->stack_size--;
            push_left(iter, node->right);
        }
    }
    return 0; // EOF
}

// ============================================================================
// DEBUG: Recursive rope invariant checker.
// Verifies weight, newlines, total_length, total_newlines, depth, and
// null-termination of every node. Aborts on first violation.
// ============================================================================
void validate_rope(RopeNode *node) {
    if (node == NULL) return;
    
    if (node->is_leaf) {
        assert(node->text != NULL && "ROPE BUG: Leaf has NULL text pointer");
        assert((int)strlen(node->text) == node->weight
               && "ROPE BUG: Leaf weight != strlen(text)");
        assert(node->total_length == node->weight
               && "ROPE BUG: Leaf total_length != weight");
        assert(node->left == NULL && "ROPE BUG: Leaf has non-NULL left child");
        assert(node->right == NULL && "ROPE BUG: Leaf has non-NULL right child");
        assert(node->depth == 1 && "ROPE BUG: Leaf depth != 1");
        
        // Independently count newlines and verify
        int nl = 0;
        for (int i = 0; i < node->weight; i++) {
            if (node->text[i] == '\n') nl++;
        }
        assert(node->newlines == nl && "ROPE BUG: Leaf newlines mismatch");
        assert(node->total_newlines == nl && "ROPE BUG: Leaf total_newlines mismatch");
        return;
    }
    
    // Internal node checks
    assert(node->text == NULL && "ROPE BUG: Internal node has non-NULL text");
    assert(node->left != NULL && "ROPE BUG: Internal node has NULL left child");
    
    // Recurse into children first
    validate_rope(node->left);
    validate_rope(node->right);
    
    int left_len = node->left->total_length;
    int right_len = node->right ? node->right->total_length : 0;
    int left_nl = node->left->total_newlines;
    int right_nl = node->right ? node->right->total_newlines : 0;
    int left_depth = node->left->depth;
    int right_depth = node->right ? node->right->depth : 0;
    
    assert(node->weight == left_len
           && "ROPE BUG: Internal weight != left->total_length");
    assert(node->newlines == left_nl
           && "ROPE BUG: Internal newlines != left->total_newlines");
    assert(node->total_length == left_len + right_len
           && "ROPE BUG: Internal total_length mismatch");
    assert(node->total_newlines == left_nl + right_nl
           && "ROPE BUG: Internal total_newlines mismatch");
    assert(node->depth == 1 + (left_depth > right_depth ? left_depth : right_depth)
           && "ROPE BUG: Internal depth mismatch");
}
