#ifndef HASH_H
#define HASH_H

#define HASH_TABLE_SIZE 128

typedef struct HashNode {
    char *key;
    int color_id;
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode *buckets[HASH_TABLE_SIZE];
} HashTable;

// Function prototypes
void hash_init(HashTable *table);
void hash_insert(HashTable *table, const char *key, int color_id);
int hash_lookup(HashTable *table, const char *key);
void hash_free(HashTable *table);

#endif
