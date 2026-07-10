#include "hash.h"
#include <stdlib.h>
#include <string.h>

static unsigned int djb2_hash(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % HASH_TABLE_SIZE;
}

void hash_init(HashTable *table) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }
}

void hash_insert(HashTable *table, const char *key, int color_id) {
    if (!key) return;
    unsigned int index = djb2_hash(key);
    
    // Check if it already exists
    HashNode *current = table->buckets[index];
    while (current) {
        if (strcmp(current->key, key) == 0) {
            current->color_id = color_id;
            return;
        }
        current = current->next;
    }
    
    // Insert new node
    HashNode *node = (HashNode*)malloc(sizeof(HashNode));
    node->key = (char*)malloc(strlen(key) + 1);
    strcpy(node->key, key);
    node->color_id = color_id;
    node->next = table->buckets[index];
    table->buckets[index] = node;
}

int hash_lookup(HashTable *table, const char *key) {
    if (!key) return -1;
    unsigned int index = djb2_hash(key);
    
    HashNode *current = table->buckets[index];
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->color_id;
        }
        current = current->next;
    }
    return -1; // Not found
}

void hash_free(HashTable *table) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashNode *current = table->buckets[i];
        while (current) {
            HashNode *temp = current;
            current = current->next;
            free(temp->key);
            free(temp);
        }
        table->buckets[i] = NULL;
    }
}
