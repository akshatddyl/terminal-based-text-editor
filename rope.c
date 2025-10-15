#include "rope.h"

// Create a leaf node with text
static RopeNode* create_leaf(const char *str) {
    RopeNode *node = (RopeNode*)malloc(sizeof(RopeNode));
    node->is_leaf = 1;
    node->weight = strlen(str);
    node->text = (char*)malloc(node->weight + 1);
    strcpy(node->text, str);
    node->left = NULL;
    node->right = NULL;
    return node;
}

// Create an internal node
static RopeNode* create_internal(RopeNode *left, RopeNode *right) {
    RopeNode *node = (RopeNode*)malloc(sizeof(RopeNode));
    node->is_leaf = 0;
    node->text = NULL;
    node->left = left;
    node->right = right;
    node->weight = rope_length(left);
    return node;
}

// Create rope from string
RopeNode* rope_create(const char *str) {
    if (str == NULL || strlen(str) == 0) {
        return create_leaf("");
    }
    return create_leaf(str);
}

// Get total length of rope
int rope_length(RopeNode *root) {
    if (root == NULL) return 0;
    if (root->is_leaf) return root->weight;
    return root->weight + rope_length(root->right);
}

// Get character at position
char rope_char_at(RopeNode *root, int position) {
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

// Convert rope to string
char* rope_to_string(RopeNode *root) {
    int len = rope_length(root);
    char *result = (char*)malloc(len + 1);
    
    if (len == 0) {
        result[0] = '\0';
        return result;
    }
    
    int index = 0;
    
    void build_string(RopeNode *node) {
        if (node == NULL) return;
        
        if (node->is_leaf) {
            strcpy(result + index, node->text);
            index += node->weight;
        } else {
            build_string(node->left);
            build_string(node->right);
        }
    }
    
    build_string(root);
    result[len] = '\0';
    return result;
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
            free(root->text);
            free(root);
        }
        return;
    }
    
    if (position < root->weight) {
        RopeNode *ll, *lr;
        rope_split(root->left, position, &ll, &lr);
        *left_out = ll;
        *right_out = create_internal(lr, root->right);
        free(root);
    } else if (position > root->weight) {
        RopeNode *rl, *rr;
        rope_split(root->right, position - root->weight, &rl, &rr);
        *left_out = create_internal(root->left, rl);
        *right_out = rr;
        free(root);
    } else {
        *left_out = root->left;
        *right_out = root->right;
        free(root);
    }
}

// Concatenate two ropes
RopeNode* rope_concat(RopeNode *left, RopeNode *right) {
    if (left == NULL) return right;
    if (right == NULL) return left;
    
    // If both are small leaves, merge them
    if (left->is_leaf && right->is_leaf && 
        left->weight + right->weight <= LEAF_SIZE) {
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

// Insert string at position
RopeNode* rope_insert(RopeNode *root, int position, const char *str) {
    if (str == NULL || strlen(str) == 0) return root;
    
    RopeNode *left, *right;
    rope_split(root, position, &left, &right);
    
    RopeNode *new_node = rope_create(str);
    RopeNode *temp = rope_concat(left, new_node);
    return rope_concat(temp, right);
}

// Delete characters from start to start+length
RopeNode* rope_delete(RopeNode *root, int start, int length) {
    if (length <= 0) return root;
    
    RopeNode *left, *mid_right;
    rope_split(root, start, &left, &mid_right);
    
    RopeNode *mid, *right;
    rope_split(mid_right, length, &mid, &right);
    
    rope_free(mid);
    return rope_concat(left, right);
}

// Free rope memory
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
