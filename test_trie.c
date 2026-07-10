#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "trie.h"

int main() {
    TrieNode *root = trie_create_node();
    
    trie_insert(root, "return");
    trie_insert(root, "struct");
    trie_insert(root, "string");
    trie_insert(root, "int");
    
    char suggestion[256];
    
    // Test prefix "ret"
    assert(trie_find_prefix(root, "ret", suggestion, 256) == true);
    assert(strcmp(suggestion, "return") == 0);
    
    // Test prefix "str"
    assert(trie_find_prefix(root, "str", suggestion, 256) == true);
    // Could be "string" or "struct" depending on DFS order, DFS goes alphabetically, so "string" first (i before u)
    assert(strcmp(suggestion, "string") == 0);
    
    // Test exact match
    assert(trie_find_prefix(root, "int", suggestion, 256) == true);
    assert(strcmp(suggestion, "int") == 0);
    
    // Test no match
    assert(trie_find_prefix(root, "float", suggestion, 256) == false);
    
    trie_free(root);
    
    printf("All trie tests passed.\n");
    return 0;
}
