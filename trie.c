#include "trie.h"
#include <stdlib.h>
#include <string.h>

TrieNode* trie_create_node(void) {
    TrieNode *node = (TrieNode*)malloc(sizeof(TrieNode));
    node->is_end_of_word = false;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        node->children[i] = NULL;
    }
    return node;
}

void trie_insert(TrieNode *root, const char *word) {
    if (!root || !word) return;
    
    TrieNode *current = root;
    for (int i = 0; word[i] != '\0'; i++) {
        unsigned char index = (unsigned char)word[i];
        if (index >= ALPHABET_SIZE) continue;
        
        if (current->children[index] == NULL) {
            current->children[index] = trie_create_node();
        }
        current = current->children[index];
    }
    current->is_end_of_word = true;
}

static bool dfs_find_first_word(TrieNode *node, char *buffer, int current_depth, int max_len) {
    if (node->is_end_of_word) {
        buffer[current_depth] = '\0';
        return true;
    }
    
    if (current_depth >= max_len - 1) {
        buffer[current_depth] = '\0';
        return false;
    }
    
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i] != NULL) {
            buffer[current_depth] = (char)i;
            if (dfs_find_first_word(node->children[i], buffer, current_depth + 1, max_len)) {
                return true;
            }
        }
    }
    
    return false;
}

bool trie_find_prefix(TrieNode *root, const char *prefix, char *suggestion_out, int max_len) {
    if (!root || !prefix || !suggestion_out || max_len <= 0) return false;
    
    TrieNode *current = root;
    int i = 0;
    
    // Traverse down the prefix
    for (; prefix[i] != '\0'; i++) {
        unsigned char index = (unsigned char)prefix[i];
        if (index >= ALPHABET_SIZE || current->children[index] == NULL) {
            return false; // Prefix doesn't exist
        }
        current = current->children[index];
    }
    
    // Copy the prefix into the buffer
    if (i >= max_len) return false;
    strncpy(suggestion_out, prefix, i);
    
    // DFS to find the first complete word from this node
    return dfs_find_first_word(current, suggestion_out, i, max_len);
}

void trie_free(TrieNode *root) {
    if (!root) return;
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (root->children[i] != NULL) {
            trie_free(root->children[i]);
        }
    }
    free(root);
}
