#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "rope.h"

// Standalone test: exercises insert, delete, split, concat, and validate_rope
// after every single mutation. If any invariant breaks, assert fires and we
// see exactly which operation caused it.

int main() {
    printf("=== Rope Invariant Stress Test ===\n\n");
    
    // Test 1: Sequential insertion at end
    printf("Test 1: Sequential insertion at end (123456789)...\n");
    RopeNode *rope = rope_create("");
    validate_rope(rope);
    for (int i = 1; i <= 9; i++) {
        char c[2] = {'0' + i, '\0'};
        rope = rope_insert(rope, rope_length(rope), c);
        validate_rope(rope);
    }
    char *s = rope_to_string(rope);
    printf("  Result: \"%s\"\n", s);
    assert(strcmp(s, "123456789") == 0 && "Sequential insert produced wrong string");
    free(s);
    printf("  PASSED.\n\n");
    
    // Test 2: Insertion in the middle
    printf("Test 2: Insert 'X' at position 4...\n");
    rope = rope_insert(rope, 4, "X");
    validate_rope(rope);
    s = rope_to_string(rope);
    printf("  Result: \"%s\"\n", s);
    assert(strcmp(s, "1234X56789") == 0 && "Middle insert produced wrong string");
    free(s);
    printf("  PASSED.\n\n");
    
    // Test 3: Deletion from the middle
    printf("Test 3: Delete 1 char at position 4 (the 'X')...\n");
    rope = rope_delete(rope, 4, 1);
    validate_rope(rope);
    s = rope_to_string(rope);
    printf("  Result: \"%s\"\n", s);
    assert(strcmp(s, "123456789") == 0 && "Middle delete produced wrong string");
    free(s);
    printf("  PASSED.\n\n");
    
    // Test 4: Insert at position 0
    printf("Test 4: Insert 'ABC' at position 0...\n");
    rope = rope_insert(rope, 0, "ABC");
    validate_rope(rope);
    s = rope_to_string(rope);
    printf("  Result: \"%s\"\n", s);
    assert(strcmp(s, "ABC123456789") == 0 && "Front insert produced wrong string");
    free(s);
    printf("  PASSED.\n\n");
    
    // Test 5: Insert with newlines, test rope_line_to_pos
    printf("Test 5: Insert newlines and verify line mapping...\n");
    rope_free(rope);
    rope = rope_create("line0\nline1\nline2");
    validate_rope(rope);
    assert(rope_newlines(rope) == 2);
    assert(rope_line_to_pos(rope, 0) == 0);
    assert(rope_line_to_pos(rope, 1) == 6);  // after "line0\n"
    assert(rope_line_to_pos(rope, 2) == 12); // after "line0\nline1\n"
    printf("  rope_line_to_pos: 0->0, 1->6, 2->12  PASSED.\n\n");
    
    // Test 6: rope_pos_to_line_col
    printf("Test 6: rope_pos_to_line_col...\n");
    int line, col;
    rope_pos_to_line_col(rope, 0, &line, &col);
    assert(line == 0 && col == 0);
    rope_pos_to_line_col(rope, 3, &line, &col);
    assert(line == 0 && col == 3);
    rope_pos_to_line_col(rope, 6, &line, &col);
    assert(line == 1 && col == 0);
    rope_pos_to_line_col(rope, 14, &line, &col);
    assert(line == 2 && col == 2);
    printf("  All pos_to_line_col checks PASSED.\n\n");
    
    // Test 7: rope_iter_seek correctness (the fixed bug)
    printf("Test 7: rope_iter_seek at every position in 100-char rope...\n");
    rope_free(rope);
    rope = rope_create("");
    for (int i = 0; i < 100; i++) {
        char c[2] = {'A' + (i % 26), '\0'};
        rope = rope_insert(rope, rope_length(rope), c);
    }
    validate_rope(rope);
    int total_len = rope_length(rope);
    
    for (int pos = 0; pos < total_len; pos++) {
        RopeIterator iter;
        int ok = rope_iter_seek(&iter, rope, pos);
        assert(ok == 1 && "Seek failed for valid position");
        char c;
        int got = rope_iter_next(&iter, &c);
        assert(got == 1 && "iter_next failed after seek");
        char expected = rope_char_at(rope, pos);
        if (c != expected) {
            fprintf(stderr, "SEEK BUG at pos %d: expected '%c' got '%c'\n", pos, expected, c);
            abort();
        }
    }
    printf("  All 100 seek+next checks matched rope_char_at.  PASSED.\n\n");
    
    // Test 8: Verify iter_seek produces no duplicate chars (suffix check)
    printf("Test 8: Verify iter_seek produces correct suffix from position 50...\n");
    {
        RopeIterator iter;
        rope_iter_seek(&iter, rope, 50);
        char buf[128] = {0};
        int idx = 0;
        char c;
        while (rope_iter_next(&iter, &c) && idx < 127) {
            buf[idx++] = c;
        }
        buf[idx] = '\0';
        assert(idx == 50 && "Suffix from position 50 should be 50 chars long");
        for (int i = 0; i < 50; i++) {
            char expected = rope_char_at(rope, 50 + i);
            if (buf[i] != expected) {
                fprintf(stderr, "SUFFIX BUG at offset %d: expected '%c' got '%c'\n", i, expected, buf[i]);
                fprintf(stderr, "Full suffix: \"%s\"\n", buf);
                abort();
            }
        }
        printf("  Suffix from pos 50: %d chars, all correct.  PASSED.\n\n", idx);
    }
    
    // Test 9: Stress test - 10000 random insertions and deletions
    printf("Test 9: 10000 random insert/delete cycles...\n");
    rope_free(rope);
    rope = rope_create("");
    for (int i = 0; i < 10000; i++) {
        int len = rope_length(rope);
        if (len > 0 && (i % 3 == 0)) {
            int pos = i % len;
            rope = rope_delete(rope, pos, 1);
        } else {
            int pos = len > 0 ? (i % (len + 1)) : 0;
            char c[2] = {'a' + (i % 26), '\0'};
            rope = rope_insert(rope, pos, c);
        }
        validate_rope(rope);
    }
    printf("  10000 operations completed. Final length: %d.  PASSED.\n\n", rope_length(rope));
    
    rope_free(rope);
    
    printf("=== ALL TESTS PASSED ===\n");
    return 0;
}
