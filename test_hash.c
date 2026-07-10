#include <stdio.h>
#include <assert.h>
#include "hash.h"

int main() {
    HashTable ht;
    hash_init(&ht);
    
    hash_insert(&ht, "int", 1);
    hash_insert(&ht, "return", 2);
    hash_insert(&ht, "struct", 3);
    
    assert(hash_lookup(&ht, "int") == 1);
    assert(hash_lookup(&ht, "return") == 2);
    assert(hash_lookup(&ht, "struct") == 3);
    assert(hash_lookup(&ht, "float") == -1);
    
    // Update existing
    hash_insert(&ht, "int", 5);
    assert(hash_lookup(&ht, "int") == 5);
    
    hash_free(&ht);
    
    printf("All hash tests passed.\n");
    return 0;
}
