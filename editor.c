#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "rope.h"
#include "undo.h"

#define CTRL(x) ((x) & 0x1f)

typedef struct
{
    RopeNode *rope;
    int cursor_x;
    int cursor_y;
    int offset_y; // Scroll offset
    char filename[256];
    int modified;
    UndoStack undo_stack;
    RedoStack redo_stack;
} Editor;

void editor_init(Editor *ed, const char *filename)
{
    ed->rope = rope_create("");
    ed->cursor_x = 0;
    ed->cursor_y = 0;
    ed->offset_y = 0;
    ed->modified = 0;
    strncpy(ed->filename, filename ? filename : "[No Name]", 255);
    undo_init(&ed->undo_stack);
    redo_init(&ed->redo_stack);
}

void editor_load_file(Editor *ed, const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *content = (char *)malloc(size + 1);
    fread(content, 1, size, fp);
    content[size] = '\0';
    fclose(fp);

    rope_free(ed->rope);
    ed->rope = rope_create(content);
    free(content);

    strncpy(ed->filename, filename, 255);
    ed->modified = 0;
}

void editor_save_file(Editor *ed)
{
    FILE *fp = fopen(ed->filename, "w");
    if (!fp)
        return;

    char *content = rope_to_string(ed->rope);
    fwrite(content, 1, strlen(content), fp);
    fclose(fp);
    free(content);

    ed->modified = 0;
}

int editor_get_position(Editor *ed)
{
    char *text = rope_to_string(ed->rope);
    int pos = 0;
    int line = 0;

    for (int i = 0; text[i] != '\0'; i++)
    {
        if (line == ed->cursor_y)
        {
            int col = 0;
            while (text[pos] != '\0' && text[pos] != '\n' && col < ed->cursor_x)
            {
                pos++;
                col++;
            }
            free(text);
            return pos;
        }
        if (text[i] == '\n')
            line++;
        pos++;
    }

    free(text);
    return pos;
}

void editor_insert_char(Editor *ed, char c)
{
    int pos = editor_get_position(ed);
    char str[2] = {c, '\0'};

    undo_push(&ed->undo_stack, OP_INSERT, pos, str, 1);
    redo_clear(&ed->redo_stack);

    ed->rope = rope_insert(ed->rope, pos, str);

    if (c == '\n')
    {
        ed->cursor_y++;
        ed->cursor_x = 0;
    }
    else
    {
        ed->cursor_x++;
    }

    ed->modified = 1;
}

void editor_delete_char(Editor *ed)
{
    if (ed->cursor_x == 0 && ed->cursor_y == 0)
        return;

    int pos = editor_get_position(ed);
    if (pos == 0)
        return;

    char deleted = rope_char_at(ed->rope, pos - 1);
    char str[2] = {deleted, '\0'};

    undo_push(&ed->undo_stack, OP_DELETE, pos - 1, str, 1);
    redo_clear(&ed->redo_stack);

    ed->rope = rope_delete(ed->rope, pos - 1, 1);

    if (deleted == '\n')
    {
        ed->cursor_y--;
        // Calculate previous line length
        char *text = rope_to_string(ed->rope);
        int line = 0, col = 0;
        for (int i = 0; text[i] != '\0' && line < ed->cursor_y; i++)
        {
            if (text[i] == '\n')
            {
                line++;
                col = 0;
            }
            else
            {
                col++;
            }
        }
        ed->cursor_x = col;
        free(text);
    }
    else
    {
        ed->cursor_x--;
    }

    ed->modified = 1;
}

void editor_undo(Editor *ed)
{
    Operation *op = undo_pop(&ed->undo_stack);
    if (!op)
        return;

    if (op->type == OP_INSERT)
    {
        // Undo insert = delete
        ed->rope = rope_delete(ed->rope, op->position, op->length);
        redo_push(&ed->redo_stack, op);
    }
    else
    {
        // Undo delete = insert
        ed->rope = rope_insert(ed->rope, op->position, op->text);
        redo_push(&ed->redo_stack, op);
    }

    ed->modified = 1;
}

void editor_redo(Editor *ed)
{
    Operation *op = redo_pop(&ed->redo_stack);
    if (!op)
        return;

    if (op->type == OP_INSERT)
    {
        ed->rope = rope_insert(ed->rope, op->position, op->text);
    }
    else
    {
        ed->rope = rope_delete(ed->rope, op->position, op->length);
    }

    ed->modified = 1;
}

void editor_move_cursor(Editor *ed, int dx, int dy)
{
    ed->cursor_y += dy;
    ed->cursor_x += dx;

    if (ed->cursor_y < 0)
        ed->cursor_y = 0;
    if (ed->cursor_x < 0)
        ed->cursor_x = 0;

    // Count lines
    char *text = rope_to_string(ed->rope);
    int lines = 1;
    for (int i = 0; text[i] != '\0'; i++)
    {
        if (text[i] == '\n')
            lines++;
    }

    if (ed->cursor_y >= lines)
        ed->cursor_y = lines - 1;

    // Clamp cursor_x to line length
    int line = 0, col = 0, line_len = 0;
    for (int i = 0; text[i] != '\0'; i++)
    {
        if (line == ed->cursor_y)
        {
            if (text[i] == '\n')
            {
                line_len = col;
                break;
            }
            col++;
        }
        if (text[i] == '\n')
        {
            if (line == ed->cursor_y - 1)
                line_len = col;
            line++;
            col = 0;
        }
    }
    if (line == ed->cursor_y)
        line_len = col;

    if (ed->cursor_x > line_len)
        ed->cursor_x = line_len;

    free(text);
}

void editor_display(Editor *ed)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    clear();

    char *text = rope_to_string(ed->rope);
    int line = 0, col = 0;
    int screen_line = 0;

    for (int i = 0; text[i] != '\0' && screen_line < rows - 1; i++)
    {
        if (line >= ed->offset_y && line < ed->offset_y + rows - 1)
        {
            if (text[i] == '\n')
            {
                screen_line++;
            }
            else if (col < cols)
            {
                mvaddch(screen_line, col, text[i]);
                col++;
            }
        }

        if (text[i] == '\n')
        {
            line++;
            col = 0;
        }
    }

    free(text);

    // Status bar
    attron(A_REVERSE);
    mvhline(rows - 1, 0, ' ', cols);
    char status[512]; // Increased buffer size
    snprintf(status, sizeof(status), " %s %s | Ln %d, Col %d | ^S:Save ^Q:Quit ^Z:Undo ^Y:Redo",
             ed->filename, ed->modified ? "[+]" : "", ed->cursor_y + 1, ed->cursor_x + 1);
    // Truncate status to fit screen width
    status[cols - 1] = '\0';
    mvprintw(rows - 1, 0, "%s", status);
    attroff(A_REVERSE);

    // Position cursor
    move(ed->cursor_y - ed->offset_y, ed->cursor_x);
    refresh();
}

int main(int argc, char *argv[])
{
    Editor ed;

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    editor_init(&ed, argc > 1 ? argv[1] : NULL);

    if (argc > 1)
    {
        editor_load_file(&ed, argv[1]);
    }

    int ch;
    while ((ch = getch()) != CTRL('q'))
    {
        switch (ch)
        {
        case KEY_UP:
            editor_move_cursor(&ed, 0, -1);
            break;
        case KEY_DOWN:
            editor_move_cursor(&ed, 0, 1);
            break;
        case KEY_LEFT:
            editor_move_cursor(&ed, -1, 0);
            break;
        case KEY_RIGHT:
            editor_move_cursor(&ed, 1, 0);
            break;
        case KEY_BACKSPACE:
        case 127:
        case '\b':
            editor_delete_char(&ed);
            break;
        case CTRL('s'):
            editor_save_file(&ed);
            break;
        case CTRL('z'):
            editor_undo(&ed);
            break;
        case CTRL('y'):
            editor_redo(&ed);
            break;
        case '\n':
        case '\r':
            editor_insert_char(&ed, '\n');
            break;
        default:
            if (ch >= 32 && ch < 127)
            {
                editor_insert_char(&ed, ch);
            }
            break;
        }

        editor_display(&ed);
    }

    endwin();
    rope_free(ed.rope);

    return 0;
}
