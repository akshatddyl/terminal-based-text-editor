#ifndef TRIE_H
#define TRIE_H

#include <stdbool.h>

#define ALPHABET_SIZE 128

typedef struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE];
    bool is_end_of_word;
} TrieNode;

// Function prototypes
TrieNode* trie_create_node(void);
void trie_insert(TrieNode *root, const char *word);
bool trie_find_prefix(TrieNode *root, const char *prefix, char *suggestion_out, int max_len);
void trie_free(TrieNode *root);

#endif
