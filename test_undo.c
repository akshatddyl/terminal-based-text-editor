#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "rope.h"
#include "undo.h"

// Standalone test: verifies undo/redo stack mechanics against the rope.
// Simulates the exact same flow as editor.c: push to undo on insert,
// pop and apply inverse on undo, pop from redo and re-apply on redo.

int main() {
    printf("=== Undo/Redo Integration Test ===\n\n");
    
    RopeNode *rope = rope_create("");
    UndoStack undo;
    RedoStack redo;
    undo_init(&undo);
    redo_init(&redo);
    
    // === Insert "Hello" one char at a time ===
    printf("Test 1: Insert 'H','e','l','l','o' and verify...\n");
    const char *hello = "Hello";
    for (int i = 0; i < 5; i++) {
        int pos = rope_length(rope);
        char s[2] = {hello[i], '\0'};
        undo_push(&undo, OP_INSERT, pos, s, 1);
        redo_clear(&redo);
        rope = rope_insert(rope, pos, s);
        validate_rope(rope);
    }
    char *str = rope_to_string(rope);
    printf("  Rope: \"%s\"\n", str);
    assert(strcmp(str, "Hello") == 0);
    free(str);
    printf("  PASSED.\n\n");
    
    // === Undo 3 times: should go from "Hello" -> "Hell" -> "Hel" -> "He" ===
    printf("Test 2: Undo 3 times...\n");
    for (int i = 0; i < 3; i++) {
        Operation *op = undo_pop(&undo);
        assert(op != NULL && "Undo stack unexpectedly empty");
        
        if (op->type == OP_INSERT) {
            assert(op->position >= 0 && op->position + op->length <= rope_length(rope));
            rope = rope_delete(rope, op->position, op->length);
        } else {
            assert(op->position >= 0 && op->position <= rope_length(rope));
            rope = rope_insert(rope, op->position, op->text);
        }
        validate_rope(rope);
        redo_push(&redo, op);
        
        str = rope_to_string(rope);
        printf("  After undo %d: \"%s\"\n", i + 1, str);
        free(str);
    }
    str = rope_to_string(rope);
    assert(strcmp(str, "He") == 0 && "After 3 undos should be 'He'");
    free(str);
    printf("  PASSED.\n\n");
    
    // === Redo 2 times: "He" -> "Hel" -> "Hell" ===
    printf("Test 3: Redo 2 times...\n");
    for (int i = 0; i < 2; i++) {
        Operation *op = redo_pop(&redo);
        assert(op != NULL && "Redo stack unexpectedly empty");
        
        if (op->type == OP_INSERT) {
            assert(op->position >= 0 && op->position <= rope_length(rope));
            rope = rope_insert(rope, op->position, op->text);
        } else {
            assert(op->position >= 0 && op->position + op->length <= rope_length(rope));
            rope = rope_delete(rope, op->position, op->length);
        }
        validate_rope(rope);
        undo_push(&undo, op->type, op->position, op->text, op->length);
        op_free(op);
        
        str = rope_to_string(rope);
        printf("  After redo %d: \"%s\"\n", i + 1, str);
        free(str);
    }
    str = rope_to_string(rope);
    assert(strcmp(str, "Hell") == 0 && "After 2 redos should be 'Hell'");
    free(str);
    printf("  PASSED.\n\n");
    
    // === New insert breaks redo chain ===
    printf("Test 4: New insert after partial undo clears redo stack...\n");
    {
        int pos = rope_length(rope); // position 4
        undo_push(&undo, OP_INSERT, pos, "!", 1);
        redo_clear(&redo); // this should free the remaining redo entry ('o')
        rope = rope_insert(rope, pos, "!");
        validate_rope(rope);
    }
    str = rope_to_string(rope);
    printf("  Rope: \"%s\"\n", str);
    assert(strcmp(str, "Hell!") == 0);
    free(str);
    // Redo stack should now be empty
    assert(redo_pop(&redo) == NULL && "Redo should be empty after new insert");
    printf("  Redo stack empty: PASSED.\n\n");
    
    // === Delete and undo the delete ===
    printf("Test 5: Delete last char '!' and undo it...\n");
    {
        int pos = rope_length(rope) - 1; // position 4 ('!')
        char deleted = rope_char_at(rope, pos);
        char ds[2] = {deleted, '\0'};
        undo_push(&undo, OP_DELETE, pos, ds, 1);
        redo_clear(&redo);
        rope = rope_delete(rope, pos, 1);
        validate_rope(rope);
        str = rope_to_string(rope);
        printf("  After delete: \"%s\"\n", str);
        assert(strcmp(str, "Hell") == 0);
        free(str);
    }
    {
        // Undo the delete (should re-insert '!')
        Operation *op = undo_pop(&undo);
        assert(op != NULL);
        assert(op->type == OP_DELETE);
        rope = rope_insert(rope, op->position, op->text);
        validate_rope(rope);
        redo_push(&redo, op);
        str = rope_to_string(rope);
        printf("  After undo delete: \"%s\"\n", str);
        assert(strcmp(str, "Hell!") == 0);
        free(str);
    }
    printf("  PASSED.\n\n");
    
    // === Stress: 500 inserts, undo all, redo all ===
    printf("Test 6: 500 inserts, undo all, redo all...\n");
    rope_free(rope);
    rope = rope_create("");
    undo_free(&undo);
    redo_free(&redo);
    undo_init(&undo);
    redo_init(&redo);
    
    for (int i = 0; i < 500; i++) {
        int pos = rope_length(rope);
        char c[2] = {'a' + (i % 26), '\0'};
        undo_push(&undo, OP_INSERT, pos, c, 1);
        rope = rope_insert(rope, pos, c);
        validate_rope(rope);
    }
    printf("  After 500 inserts: length=%d\n", rope_length(rope));
    assert(rope_length(rope) == 500);
    
    // Undo all (capped at MAX_UNDO=100)
    int undo_count = 0;
    while (1) {
        Operation *op = undo_pop(&undo);
        if (!op) break;
        if (op->type == OP_INSERT) {
            rope = rope_delete(rope, op->position, op->length);
        } else {
            rope = rope_insert(rope, op->position, op->text);
        }
        validate_rope(rope);
        redo_push(&redo, op);
        undo_count++;
    }
    printf("  Undid %d operations. Rope length: %d\n", undo_count, rope_length(rope));
    assert(undo_count == 100 && "Should undo exactly MAX_UNDO=100 operations");
    assert(rope_length(rope) == 400 && "500 - 100 undos = 400");
    
    // Redo all
    int redo_count = 0;
    while (1) {
        Operation *op = redo_pop(&redo);
        if (!op) break;
        if (op->type == OP_INSERT) {
            rope = rope_insert(rope, op->position, op->text);
        } else {
            rope = rope_delete(rope, op->position, op->length);
        }
        validate_rope(rope);
        undo_push(&undo, op->type, op->position, op->text, op->length);
        op_free(op);
        redo_count++;
    }
    printf("  Redid %d operations. Rope length: %d\n", redo_count, rope_length(rope));
    assert(redo_count == 100);
    assert(rope_length(rope) == 500);
    printf("  PASSED.\n\n");
    
    rope_free(rope);
    undo_free(&undo);
    redo_free(&redo);
    
    printf("=== ALL UNDO/REDO TESTS PASSED ===\n");
    return 0;
}
