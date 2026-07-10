#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "rope.h"

#define OPERATIONS 100000

// Naive buffer structure
typedef struct {
    char *text;
    int length;
    int capacity;
} NaiveBuffer;

void naive_init(NaiveBuffer *buf) {
    buf->capacity = 1024;
    buf->length = 0;
    buf->text = (char*)malloc(buf->capacity);
    buf->text[0] = '\0';
}

void naive_insert(NaiveBuffer *buf, int pos, const char *str) {
    int len = strlen(str);
    if (buf->length + len >= buf->capacity) {
        while (buf->length + len >= buf->capacity) buf->capacity *= 2;
        buf->text = (char*)realloc(buf->text, buf->capacity);
    }
    
    if (pos < buf->length) {
        memmove(buf->text + pos + len, buf->text + pos, buf->length - pos + 1);
    }
    memcpy(buf->text + pos, str, len);
    buf->length += len;
    buf->text[buf->length] = '\0';
}

void naive_delete(NaiveBuffer *buf, int pos, int len) {
    if (pos >= buf->length) return;
    if (pos + len >= buf->length) {
        buf->length = pos;
        buf->text[pos] = '\0';
    } else {
        memmove(buf->text + pos, buf->text + pos + len, buf->length - (pos + len) + 1);
        buf->length -= len;
    }
}

void naive_free(NaiveBuffer *buf) {
    free(buf->text);
}

int main() {
    printf("Starting Benchmark: %d insertions/deletions\n", OPERATIONS);
    
    // 1. Benchmark Naive Buffer
    NaiveBuffer naive;
    naive_init(&naive);
    
    clock_t start = clock();
    for (int i = 0; i < OPERATIONS; i++) {
        // Always insert at the middle to simulate average case
        int pos = naive.length / 2;
        naive_insert(&naive, pos, "a");
    }
    for (int i = 0; i < OPERATIONS; i++) {
        int pos = naive.length / 2;
        naive_delete(&naive, pos, 1);
    }
    clock_t end = clock();
    double naive_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    naive_free(&naive);
    
    printf("Naive String Buffer Time: %.4f seconds\n", naive_time);
    
    // 2. Benchmark Rope
    RopeNode *rope = rope_create("");
    start = clock();
    for (int i = 0; i < OPERATIONS; i++) {
        int pos = rope_length(rope) / 2;
        rope = rope_insert(rope, pos, "a");
    }
    for (int i = 0; i < OPERATIONS; i++) {
        int pos = rope_length(rope) / 2;
        rope = rope_delete(rope, pos, 1);
    }
    end = clock();
    double rope_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    rope_free(rope);
    
    printf("Rope Data Structure Time: %.4f seconds\n", rope_time);
    
    if (rope_time < naive_time) {
        printf("Rope is %.2fx faster!\n", naive_time / rope_time);
    } else {
        printf("Rope overhead makes it slower for this threshold (%.2fx slower).\n", rope_time / naive_time);
    }
    
    return 0;
}
