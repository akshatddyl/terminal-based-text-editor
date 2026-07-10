#ifndef SEARCH_H
#define SEARCH_H

#include "rope.h"

// Search for the first occurrence of pattern in the rope, starting from start_pos.
// Returns the character position of the match, or -1 if not found.
int bm_search(const RopeNode *rope, int start_pos, const char *pattern);

#endif
