#include "search.h"
#include <string.h>

#define MAX_CHAR 256

static inline int max(int a, int b) {
    return (a > b) ? a : b;
}

// Preprocess the bad character heuristic
static void bad_char_heuristic(const char *str, int size, int badchar[MAX_CHAR]) {
    for (int i = 0; i < MAX_CHAR; i++) {
        badchar[i] = -1;
    }
    for (int i = 0; i < size; i++) {
        badchar[(unsigned char)str[i]] = i;
    }
}

int bm_search(const RopeNode *rope, int start_pos, const char *pattern) {
    if (!rope || !pattern) return -1;
    
    int m = strlen(pattern);
    int n = rope_length(rope);
    
    if (m == 0 || n == 0 || start_pos >= n) return -1;
    
    int badchar[MAX_CHAR];
    bad_char_heuristic(pattern, m, badchar);
    
    int s = start_pos;
    while (s <= (n - m)) {
        int j = m - 1;
        
        // Rope character lookup is O(log N).
        // Since Boyer-Moore skips rapidly, this is generally fine.
        // For absolute peak performance, a buffered iterator could be used, 
        // but O(log N) per char match is perfectly acceptable for editor search.
        while (j >= 0 && pattern[j] == rope_char_at(rope, s + j)) {
            j--;
        }
        
        if (j < 0) {
            return s; // Match found
        } else {
            char bad_c = rope_char_at(rope, s + j);
            s += max(1, j - badchar[(unsigned char)bad_c]);
        }
    }
    
    return -1; // No match found
}
