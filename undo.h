#ifndef UNDO_H
#define UNDO_H

#define MAX_UNDO 100

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
    Operation *operations[MAX_UNDO];
    int head;
    int tail;
    int count;
} UndoStack;

typedef struct {
    Operation *operations[MAX_UNDO];
    int top;
} RedoStack;

// Function prototypes
void undo_init(UndoStack *stack);
void redo_init(RedoStack *stack);

// Push creates a new operation and copies the text.
void undo_push(UndoStack *stack, OperationType type, int position, const char *text, int length);

// Pop removes the operation from the stack and transfers ownership to the caller.
Operation* undo_pop(UndoStack *stack);

// Redo push takes ownership of the given operation.
void redo_push(RedoStack *stack, Operation *op);

// Redo pop removes from the stack and transfers ownership back to the caller.
Operation* redo_pop(RedoStack *stack);

// Clear frees all operations in the redo stack.
void redo_clear(RedoStack *stack);

void undo_free(UndoStack *stack);
void redo_free(RedoStack *stack);
void op_free(Operation *op);

#endif
