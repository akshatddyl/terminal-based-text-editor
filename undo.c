#include "undo.h"
#include <string.h>
#include <stdlib.h>

void undo_init(UndoStack *stack) {
    stack->top = -1;
}

void redo_init(RedoStack *stack) {
    stack->top = -1;
}

void undo_push(UndoStack *stack, OperationType type, int position, const char *text, int length) {
    if (stack->top >= MAX_UNDO - 1) {
        // Remove oldest operation
        free(stack->operations[0].text);
        for (int i = 0; i < MAX_UNDO - 1; i++) {
            stack->operations[i] = stack->operations[i + 1];
        }
        stack->top--;
    }
    
    stack->top++;
    stack->operations[stack->top].type = type;
    stack->operations[stack->top].position = position;
    stack->operations[stack->top].length = length;
    
    if (text != NULL) {
        stack->operations[stack->top].text = (char*)malloc(strlen(text) + 1);
        strcpy(stack->operations[stack->top].text, text);
    } else {
        stack->operations[stack->top].text = NULL;
    }
}

Operation* undo_pop(UndoStack *stack) {
    if (stack->top < 0) return NULL;
    return &stack->operations[stack->top--];
}

void redo_push(RedoStack *stack, Operation *op) {
    if (stack->top >= MAX_UNDO - 1) return;
    
    stack->top++;
    stack->operations[stack->top] = *op;
    
    if (op->text != NULL) {
        stack->operations[stack->top].text = (char*)malloc(strlen(op->text) + 1);
        strcpy(stack->operations[stack->top].text, op->text);
    }
}

Operation* redo_pop(RedoStack *stack) {
    if (stack->top < 0) return NULL;
    return &stack->operations[stack->top--];
}

void redo_clear(RedoStack *stack) {
    while (stack->top >= 0) {
        if (stack->operations[stack->top].text != NULL) {
            free(stack->operations[stack->top].text);
        }
        stack->top--;
    }
}

int undo_is_empty(UndoStack *stack) {
    return stack->top < 0;
}

int redo_is_empty(RedoStack *stack) {
    return stack->top < 0;
}
