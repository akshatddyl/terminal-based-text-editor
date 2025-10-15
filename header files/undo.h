#ifndef UNDO_H
#define UNDO_H

#include "rope.h"

#define MAX_UNDO 50

typedef enum {
    OP_INSERT,
    OP_DELETE
} OperationType;

typedef struct {
    OperationType type;
    int position;
    char *text;
    int length;
} Operation;

typedef struct {
    Operation operations[MAX_UNDO];
    int top;
} UndoStack;

typedef struct {
    Operation operations[MAX_UNDO];
    int top;
} RedoStack;

// Function prototypes
void undo_init(UndoStack *stack);
void redo_init(RedoStack *stack);
void undo_push(UndoStack *stack, OperationType type, int position, const char *text, int length);
Operation* undo_pop(UndoStack *stack);
void redo_push(RedoStack *stack, Operation *op);
Operation* redo_pop(RedoStack *stack);
void redo_clear(RedoStack *stack);
int undo_is_empty(UndoStack *stack);
int redo_is_empty(RedoStack *stack);

#endif
