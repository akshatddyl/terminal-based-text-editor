#include "undo.h"
#include <string.h>
#include <stdlib.h>

void op_free(Operation *op) {
    if (!op) return;
    if (op->text) free(op->text);
    free(op);
}

void undo_init(UndoStack *stack) {
    stack->head = 0;
    stack->tail = 0;
    stack->count = 0;
}

void redo_init(RedoStack *stack) {
    stack->top = -1;
}

void undo_push(UndoStack *stack, OperationType type, int position, const char *text, int length) {
    Operation *op = (Operation*)malloc(sizeof(Operation));
    op->type = type;
    op->position = position;
    op->length = length;
    if (text) {
        op->text = (char*)malloc(strlen(text) + 1);
        strcpy(op->text, text);
    } else {
        op->text = NULL;
    }

    if (stack->count == MAX_UNDO) {
        // Buffer is full, drop the oldest operation
        op_free(stack->operations[stack->head]);
        stack->head = (stack->head + 1) % MAX_UNDO;
        stack->count--;
    }

    stack->operations[stack->tail] = op;
    stack->tail = (stack->tail + 1) % MAX_UNDO;
    stack->count++;
}

Operation* undo_pop(UndoStack *stack) {
    if (stack->count == 0) return NULL;
    
    // Tail points to the next write pos, so the last inserted is (tail - 1)
    int last = (stack->tail - 1 + MAX_UNDO) % MAX_UNDO;
    Operation *op = stack->operations[last];
    
    stack->tail = last;
    stack->count--;
    return op;
}

void redo_push(RedoStack *stack, Operation *op) {
    if (stack->top >= MAX_UNDO - 1) {
        // Redo stack is full, drop the oldest (bottom of stack)
        op_free(stack->operations[0]);
        for (int i = 0; i < MAX_UNDO - 1; i++) {
            stack->operations[i] = stack->operations[i + 1];
        }
        stack->top--;
    }
    
    stack->top++;
    stack->operations[stack->top] = op;
}

Operation* redo_pop(RedoStack *stack) {
    if (stack->top < 0) return NULL;
    return stack->operations[stack->top--];
}

void redo_clear(RedoStack *stack) {
    while (stack->top >= 0) {
        op_free(stack->operations[stack->top]);
        stack->top--;
    }
}

void undo_free(UndoStack *stack) {
    while (stack->count > 0) {
        op_free(stack->operations[stack->head]);
        stack->head = (stack->head + 1) % MAX_UNDO;
        stack->count--;
    }
}

void redo_free(RedoStack *stack) {
    redo_clear(stack);
}
