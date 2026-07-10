#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>
#include "rope.h"
#include "undo.h"
#include "trie.h"
#include "hash.h"
#include "search.h"

#define CTRL(x) ((x) & 0x1f)

#define COLOR_KEYWORD 1
#define COLOR_TYPE    2
#define COLOR_MACRO   3

typedef enum {
    MODE_NORMAL,
    MODE_SEARCH,
    MODE_REPLACE,
    MODE_PROMPT
} EditorMode;

typedef struct {
    RopeNode *rope;
    int cursor_x;
    int cursor_y;
    int offset_y;
    UndoStack undo_stack;
    RedoStack redo_stack;
    char filename[256];
    int modified;
    WINDOW *win;
    int win_rows;
    int win_cols;
} EditorView;

typedef struct {
    EditorView views[2];
    int num_views;
    int active_view;
    
    TrieNode *trie;
    HashTable hash_table;
    char current_suggestion[256];
    char current_prefix[256];
    
    EditorMode mode;
    char prompt_text[256];
    char prompt_buffer[256];
    int prompt_len;
    int prompt_action;
    char search_query[256];
    
    int last_search_pos;
    int last_search_len;
    char status_msg[256];
} EditorApp;

// Function prototypes
void view_display(EditorApp *app, int view_idx);
void view_insert_char(EditorApp *app, EditorView *view, char c);
void view_delete_char(EditorApp *app, EditorView *view);
int view_get_position(EditorView *view);
void view_auto_save(EditorView *view);

// ============================================================================
// FOOLPROOF INDEX MAPPING: 2D (cy, cx) -> 1D rope byte offset.
// Uses ONLY rope_iter_init + rope_iter_next (never rope_iter_seek).
// This avoids any iterator stack corruption by using the known-good
// in-order traversal path.
// Also clamps cx to the actual line length via out_clamped_cx.
// ============================================================================
static int get_rope_index(int cy, int cx, RopeNode *rope, int *out_clamped_cx) {
    assert(cy >= 0 && cx >= 0);
    int total_len = rope_length(rope);
    
    if (total_len == 0) {
        if (out_clamped_cx) *out_clamped_cx = 0;
        return 0;
    }
    
    RopeIterator iter;
    rope_iter_init(&iter, rope);
    char c;
    int pos = 0;
    int cur_line = 0;
    
    // Phase 1: Walk forward character-by-character until we reach line cy.
    // Each '\n' we pass increments cur_line.
    while (cur_line < cy) {
        if (!rope_iter_next(&iter, &c)) {
            // Ran out of text before reaching target line.
            if (out_clamped_cx) *out_clamped_cx = 0;
            assert(pos <= total_len);
            return pos;
        }
        pos++;
        if (c == '\n') cur_line++;
    }
    // pos is now at the first character of line cy.
    
    // Phase 2: Measure the length of line cy by copying the iterator
    // state and scanning forward to the next '\n' or EOF.
    RopeIterator scan_iter = iter; // value copy — safe since it's a plain struct
    int line_len = 0;
    while (rope_iter_next(&scan_iter, &c) && c != '\n') {
        line_len++;
    }
    
    int clamped_cx = (cx > line_len) ? line_len : cx;
    if (out_clamped_cx) *out_clamped_cx = clamped_cx;
    
    int result = pos + clamped_cx;
    assert(result >= 0 && result <= total_len);
    return result;
}

void app_init_keywords(EditorApp *app) {
    const char *keywords[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "int", "long", "register", "return", "short", "signed", "sizeof", "static",
        "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while",
        "#include", "#define", "#ifndef", "#endif", "#ifdef"
    };
    int num_keywords = sizeof(keywords) / sizeof(keywords[0]);
    
    app->trie = trie_create_node();
    hash_init(&app->hash_table);
    
    for (int i = 0; i < num_keywords; i++) {
        trie_insert(app->trie, keywords[i]);
        int color = COLOR_KEYWORD;
        if (strcmp(keywords[i], "int") == 0 || strcmp(keywords[i], "char") == 0 || 
            strcmp(keywords[i], "float") == 0 || strcmp(keywords[i], "double") == 0 ||
            strcmp(keywords[i], "void") == 0 || strcmp(keywords[i], "long") == 0 ||
            strcmp(keywords[i], "short") == 0 || strcmp(keywords[i], "struct") == 0) {
            color = COLOR_TYPE;
        } else if (keywords[i][0] == '#') {
            color = COLOR_MACRO;
        }
        hash_insert(&app->hash_table, keywords[i], color);
    }
}

void view_init(EditorView *view, const char *filename, WINDOW *win, int rows, int cols) {
    view->win = win;
    view->win_rows = rows;
    view->win_cols = cols;
    view->rope = rope_create("");
    view->cursor_x = 0;
    view->cursor_y = 0;
    view->offset_y = 0;
    undo_init(&view->undo_stack);
    redo_init(&view->redo_stack);
    view->modified = 0;
    
    if (filename) {
        strncpy(view->filename, filename, 255);
        view->filename[255] = '\0';
        char swp_file[300];
        snprintf(swp_file, sizeof(swp_file), ".%s.swp", filename);
        FILE *fp = fopen(swp_file, "r");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            long size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            char *content = malloc(size + 1);
            fread(content, 1, size, fp);
            content[size] = '\0';
            fclose(fp);
            rope_free(view->rope);
            view->rope = rope_create(content);
            free(content);
            view->modified = 1;
        } else {
            fp = fopen(filename, "r");
            if (fp) {
                fseek(fp, 0, SEEK_END);
                long size = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                char *content = malloc(size + 1);
                fread(content, 1, size, fp);
                content[size] = '\0';
                fclose(fp);
                rope_free(view->rope);
                view->rope = rope_create(content);
                free(content);
            }
        }
    } else {
        view->filename[0] = '\0';
    }
}

void app_init(EditorApp *app, int argc, char *argv[]) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    
    app_init_keywords(app);
    app->current_suggestion[0] = '\0';
    app->current_prefix[0] = '\0';
    app->mode = MODE_NORMAL;
    app->prompt_len = 0;
    app->last_search_pos = -1;
    app->last_search_len = 0;
    app->status_msg[0] = '\0';
    
    if (argc > 2) {
        app->num_views = 2;
        app->active_view = 0;
        int half_rows = rows / 2;
        
        WINDOW *w1 = newwin(half_rows, cols, 0, 0);
        WINDOW *w2 = newwin(rows - half_rows - 1, cols, half_rows, 0);
        
        view_init(&app->views[0], argv[1], w1, half_rows, cols);
        view_init(&app->views[1], argv[2], w2, rows - half_rows - 1, cols);
    } else {
        app->num_views = 1;
        app->active_view = 0;
        WINDOW *w1 = newwin(rows - 1, cols, 0, 0);
        view_init(&app->views[0], argc > 1 ? argv[1] : NULL, w1, rows - 1, cols);
    }
}

void view_auto_save(EditorView *view) {
    if (strlen(view->filename) == 0) return;
    char swp_file[300];
    snprintf(swp_file, sizeof(swp_file), ".%s.swp", view->filename);
    FILE *fp = fopen(swp_file, "w");
    if (!fp) return;
    char *text = rope_to_string(view->rope);
    fwrite(text, 1, rope_length(view->rope), fp);
    fclose(fp);
    free(text);
}

void view_save_file(EditorView *view) {
    if (strlen(view->filename) == 0) return;
    char tmp_file[300];
    snprintf(tmp_file, sizeof(tmp_file), "%s.tmp", view->filename);
    FILE *fp = fopen(tmp_file, "w");
    if (!fp) return;
    char *text = rope_to_string(view->rope);
    fwrite(text, 1, rope_length(view->rope), fp);
    fclose(fp);
    free(text);
    rename(tmp_file, view->filename);
    view->modified = 0;
    
    char swp_file[300];
    snprintf(swp_file, sizeof(swp_file), ".%s.swp", view->filename);
    remove(swp_file);
}

// ============================================================================
// Uses get_rope_index for all 2D -> 1D mapping. Clamps cursor_x as a side effect.
// ============================================================================
int view_get_position(EditorView *view) {
    int clamped_cx;
    int pos = get_rope_index(view->cursor_y, view->cursor_x, view->rope, &clamped_cx);
    view->cursor_x = clamped_cx;
    return pos;
}

void app_update_suggestion(EditorApp *app, EditorView *view) {
    app->current_suggestion[0] = '\0';
    app->current_prefix[0] = '\0';
    if (view->cursor_x == 0) return;
    
    int line_start_pos = get_rope_index(view->cursor_y, 0, view->rope, NULL);
    RopeIterator iter;
    rope_iter_init(&iter, view->rope);
    // Skip to line_start_pos
    char c;
    for (int i = 0; i < line_start_pos; i++) {
        rope_iter_next(&iter, &c);
    }
    
    int col = 0;
    char prefix[256] = {0};
    int prefix_len = 0;
    
    while (col < view->cursor_x && rope_iter_next(&iter, &c)) {
        if (isalnum(c) || c == '_' || c == '#') {
            if (prefix_len < 255) {
                prefix[prefix_len++] = c;
                prefix[prefix_len] = '\0';
            }
        } else {
            prefix_len = 0;
            prefix[0] = '\0';
        }
        col++;
    }
    
    if (prefix_len > 0) {
        strcpy(app->current_prefix, prefix);
        trie_find_prefix(app->trie, prefix, app->current_suggestion, 255);
    }
}

void view_insert_char(EditorApp *app, EditorView *view, char c) {
    int pos = view_get_position(view);
    char str[2] = {c, '\0'};
    
    undo_push(&view->undo_stack, OP_INSERT, pos, str, 1);
    redo_clear(&view->redo_stack);
    
    view->rope = rope_insert(view->rope, pos, str);
    validate_rope(view->rope);

    if (c == '\n') {
        view->cursor_y++;
        view->cursor_x = 0;
        
        // Auto-indentation: measure leading whitespace of previous line
        int prev_line = view->cursor_y - 1;
        int prev_start = get_rope_index(prev_line, 0, view->rope, NULL);
        RopeIterator iter;
        rope_iter_init(&iter, view->rope);
        char pc;
        for (int i = 0; i < prev_start; i++) rope_iter_next(&iter, &pc);
        
        int spaces = 0;
        char last_char = '\0';
        
        while (rope_iter_next(&iter, &pc) && pc != '\n') {
            if (pc == ' ' && last_char == '\0') spaces++;
            else if (pc != ' ') last_char = pc;
        }
        
        if (last_char == '{') spaces += 4;
        
        for (int i = 0; i < spaces; i++) {
            char s[2] = {' ', '\0'};
            int new_pos = view_get_position(view);
            view->rope = rope_insert(view->rope, new_pos, s);
            validate_rope(view->rope);
            undo_push(&view->undo_stack, OP_INSERT, new_pos, s, 1);
            view->cursor_x++;
        }
        
    } else {
        view->cursor_x++;
    }
    view->modified = 1;
    view_auto_save(view);
}

void view_delete_char(EditorApp *app, EditorView *view) {
    if (view->cursor_x == 0 && view->cursor_y == 0) return;
    int pos = view_get_position(view);
    if (pos == 0) return;
    char deleted = rope_char_at(view->rope, pos - 1);
    char str[2] = {deleted, '\0'};
    
    undo_push(&view->undo_stack, OP_DELETE, pos - 1, str, 1);
    redo_clear(&view->redo_stack);
    
    view->rope = rope_delete(view->rope, pos - 1, 1);
    validate_rope(view->rope);

    if (deleted == '\n') {
        view->cursor_y--;
        int clamped_cx;
        get_rope_index(view->cursor_y, 99999, view->rope, &clamped_cx);
        view->cursor_x = clamped_cx;
    } else {
        view->cursor_x--;
    }
    view->modified = 1;
    view_auto_save(view);
}

// ============================================================================
// UNDO: Pop from undo stack, apply the INVERSE operation to the rope,
// move cursor to the affected position, push original onto redo stack.
// ============================================================================
void view_undo(EditorView *view) {
    Operation *op = undo_pop(&view->undo_stack);
    if (!op) return;
    
    if (op->type == OP_INSERT) {
        // Original action inserted text at op->position.
        // Inverse: delete that text.
        assert(op->position >= 0 && op->position + op->length <= rope_length(view->rope));
        view->rope = rope_delete(view->rope, op->position, op->length);
        validate_rope(view->rope);
        // Place cursor at the position where text was removed
        rope_pos_to_line_col(view->rope, op->position, &view->cursor_y, &view->cursor_x);
    } else { // OP_DELETE
        // Original action deleted text from op->position.
        // Inverse: re-insert that text.
        assert(op->position >= 0 && op->position <= rope_length(view->rope));
        view->rope = rope_insert(view->rope, op->position, op->text);
        validate_rope(view->rope);
        // Place cursor at the end of the re-inserted text
        rope_pos_to_line_col(view->rope, op->position + op->length, &view->cursor_y, &view->cursor_x);
    }
    
    // Transfer ownership of this operation to the redo stack
    redo_push(&view->redo_stack, op);
    view->modified = 1;
    view_auto_save(view);
}

// ============================================================================
// REDO: Pop from redo stack, RE-APPLY the original operation to the rope,
// move cursor, push a copy back onto the undo stack.
// ============================================================================
void view_redo(EditorView *view) {
    Operation *op = redo_pop(&view->redo_stack);
    if (!op) return;
    
    if (op->type == OP_INSERT) {
        // Re-apply: insert the text again
        assert(op->position >= 0 && op->position <= rope_length(view->rope));
        view->rope = rope_insert(view->rope, op->position, op->text);
        validate_rope(view->rope);
        // Place cursor at the end of the re-inserted text
        rope_pos_to_line_col(view->rope, op->position + op->length, &view->cursor_y, &view->cursor_x);
    } else { // OP_DELETE
        // Re-apply: delete the text again
        assert(op->position >= 0 && op->position + op->length <= rope_length(view->rope));
        view->rope = rope_delete(view->rope, op->position, op->length);
        validate_rope(view->rope);
        // Place cursor at the deletion point
        rope_pos_to_line_col(view->rope, op->position, &view->cursor_y, &view->cursor_x);
    }
    
    // Push a copy back onto the undo stack (undo_push copies internally)
    undo_push(&view->undo_stack, op->type, op->position, op->text, op->length);
    op_free(op);
    view->modified = 1;
    view_auto_save(view);
}

void view_move_cursor(EditorView *view, int dx, int dy) {
    int total_lines = rope_newlines(view->rope) + 1;
    view->cursor_y += dy;
    if (view->cursor_y < 0) view->cursor_y = 0;
    if (view->cursor_y >= total_lines) view->cursor_y = total_lines - 1;
    view->cursor_x += dx;
    if (view->cursor_x < 0) view->cursor_x = 0;
    
    // Clamp cursor_x to the actual line length via get_rope_index
    int clamped_cx;
    get_rope_index(view->cursor_y, view->cursor_x, view->rope, &clamped_cx);
    view->cursor_x = clamped_cx;
}

void view_display(EditorApp *app, int view_idx) {
    EditorView *view = &app->views[view_idx];
    WINDOW *win = view->win;
    wclear(win);
    
    if (app->active_view == view_idx) {
        if (view->cursor_y < view->offset_y) view->offset_y = view->cursor_y;
        if (view->cursor_y >= view->offset_y + view->win_rows - 1) view->offset_y = view->cursor_y - view->win_rows + 2;
    }
    
    int start_pos = get_rope_index(view->offset_y, 0, view->rope, NULL);
    int current_pos = start_pos;
    
    if (view->rope) {
        RopeIterator iter;
        rope_iter_seek(&iter, view->rope, start_pos);
        
        int screen_line = 0;
        int col = 0;
        char c;
        
        char word_buf[256];
        int word_len = 0;
        int word_start_col = 0;
        
        while (rope_iter_next(&iter, &c) && screen_line < view->win_rows - 1) {
            if (c == '\n') {
                if (word_len > 0) {
                    word_buf[word_len] = '\0';
                    int color = hash_lookup(&app->hash_table, word_buf);
                    if (color > 0) wattron(win, COLOR_PAIR(color));
                    for (int i = 0; i < word_len; i++) {
                        int char_pos = current_pos - word_len + i;
                        int in_search = (app->last_search_pos != -1 && char_pos >= app->last_search_pos && char_pos < app->last_search_pos + app->last_search_len);
                        if (in_search) wattron(win, A_REVERSE);
                        if (word_start_col + i < view->win_cols) mvwaddch(win, screen_line, word_start_col + i, word_buf[i]);
                        if (in_search) wattroff(win, A_REVERSE);
                    }
                    if (color > 0) wattroff(win, COLOR_PAIR(color));
                    word_len = 0;
                }
                screen_line++;
                col = 0;
            } else {
                if (isalnum(c) || c == '_' || c == '#') {
                    if (word_len == 0) word_start_col = col;
                    if (word_len < 255) word_buf[word_len++] = c;
                } else {
                    if (word_len > 0) {
                        word_buf[word_len] = '\0';
                        int color = hash_lookup(&app->hash_table, word_buf);
                        if (color > 0) wattron(win, COLOR_PAIR(color));
                        for (int i = 0; i < word_len; i++) {
                            int char_pos = current_pos - word_len + i;
                            int in_search = (app->last_search_pos != -1 && char_pos >= app->last_search_pos && char_pos < app->last_search_pos + app->last_search_len);
                            if (in_search) wattron(win, A_REVERSE);
                            if (word_start_col + i < view->win_cols) mvwaddch(win, screen_line, word_start_col + i, word_buf[i]);
                            if (in_search) wattroff(win, A_REVERSE);
                        }
                        if (color > 0) wattroff(win, COLOR_PAIR(color));
                        word_len = 0;
                    }
                    
                    int in_search = (app->last_search_pos != -1 && current_pos >= app->last_search_pos && current_pos < app->last_search_pos + app->last_search_len);
                    if (in_search) wattron(win, A_REVERSE);
                    if (col < view->win_cols) mvwaddch(win, screen_line, col, c);
                    if (in_search) wattroff(win, A_REVERSE);
                }
                col++;
            }
            current_pos++;
        }
        // Flush any trailing word
        if (word_len > 0 && screen_line < view->win_rows - 1) {
            word_buf[word_len] = '\0';
            int color = hash_lookup(&app->hash_table, word_buf);
            if (color > 0) wattron(win, COLOR_PAIR(color));
            for (int i = 0; i < word_len; i++) {
                int char_pos = current_pos - word_len + i;
                int in_search = (app->last_search_pos != -1 && char_pos >= app->last_search_pos && char_pos < app->last_search_pos + app->last_search_len);
                if (in_search) wattron(win, A_REVERSE);
                if (word_start_col + i < view->win_cols) mvwaddch(win, screen_line, word_start_col + i, word_buf[i]);
                if (in_search) wattroff(win, A_REVERSE);
            }
            if (color > 0) wattroff(win, COLOR_PAIR(color));
        }
    }
    
    // Status line
    wattron(win, A_REVERSE);
    mvwhline(win, view->win_rows - 1, 0, ' ', view->win_cols);
    char status[256];
    const char *mode_str = "NORMAL";
    if (app->mode == MODE_SEARCH) mode_str = "SEARCH";
    if (app->mode == MODE_REPLACE) mode_str = "REPLACE";
    
    snprintf(status, sizeof(status), " [%s] %s %s %s | Ln %d, Col %d",
             mode_str,
             app->active_view == view_idx ? "*" : " ",
             strlen(view->filename) > 0 ? view->filename : "[No Name]",
             view->modified ? "[+]" : "",
             view->cursor_y + 1, view->cursor_x + 1);
             
    mvwprintw(win, view->win_rows - 1, 0, "%s", status);
    wattroff(win, A_REVERSE);
}

void app_display(EditorApp *app) {
    EditorView *active = &app->views[app->active_view];
    app_update_suggestion(app, active);
    
    for (int i = 0; i < app->num_views; i++) {
        view_display(app, i);
        wrefresh(app->views[i].win);
    }
    
    // Global Status / Prompt
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    attron(A_REVERSE);
    mvhline(rows - 1, 0, ' ', cols);
    
    if (app->mode == MODE_PROMPT) {
        mvprintw(rows - 1, 0, "%s %s", app->prompt_text, app->prompt_buffer);
    } else {
        char sugg_text[256] = "";
        if (strlen(app->current_suggestion) > 0) {
            snprintf(sugg_text, sizeof(sugg_text), " [Sugg: %s (TAB)]", app->current_suggestion);
        }
        
        if (strlen(app->status_msg) > 0) {
            mvprintw(rows - 1, 0, " %s", app->status_msg);
        } else {
            mvprintw(rows - 1, 0, " ^W:Swap ^F:Find ^R:Replace ^S:Save ^Q:Quit%s", sugg_text);
        }
    }
    attroff(A_REVERSE);
    
    refresh();
    
    if (app->mode == MODE_PROMPT) {
        move(rows - 1, (int)strlen(app->prompt_text) + 1 + app->prompt_len);
        refresh();
    } else {
        wmove(active->win, active->cursor_y - active->offset_y, active->cursor_x);
        wrefresh(active->win);
    }
}

int main(int argc, char *argv[]) {
    EditorApp app;

    initscr();
    start_color();
    use_default_colors();
    init_pair(COLOR_KEYWORD, COLOR_YELLOW, -1);
    init_pair(COLOR_TYPE, COLOR_GREEN, -1);
    init_pair(COLOR_MACRO, COLOR_BLUE, -1);
    
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(1);

    app_init(&app, argc, argv);
    int ch;
    app_display(&app);

    while ((ch = getch()) != CTRL('q')) {
        EditorView *view = &app.views[app.active_view];
        
        if (app.mode != MODE_PROMPT) {
            app.status_msg[0] = '\0';
        }
        
        if (app.mode == MODE_PROMPT) {
            if (ch == '\n' || ch == '\r') {
                app.prompt_buffer[app.prompt_len] = '\0';
                if (app.prompt_action == 1) { // Search
                    int pos = bm_search(view->rope, view_get_position(view), app.prompt_buffer);
                    if (pos == -1 && view_get_position(view) > 0) {
                        pos = bm_search(view->rope, 0, app.prompt_buffer);
                    }
                    if (pos != -1) {
                        rope_pos_to_line_col(view->rope, pos, &view->cursor_y, &view->cursor_x);
                        app.last_search_pos = pos;
                        app.last_search_len = strlen(app.prompt_buffer);
                        snprintf(app.status_msg, sizeof(app.status_msg), "Found: %s", app.prompt_buffer);
                    } else {
                        app.last_search_pos = -1;
                        app.last_search_len = 0;
                        snprintf(app.status_msg, sizeof(app.status_msg), "Search: %s not found", app.prompt_buffer);
                    }
                    app.mode = MODE_NORMAL;
                } else if (app.prompt_action == 2) { // Replace - got search term
                    strcpy(app.search_query, app.prompt_buffer);
                    strcpy(app.prompt_text, "Replace with:");
                    app.prompt_len = 0;
                    app.prompt_buffer[0] = '\0';
                    app.prompt_action = 3;
                } else if (app.prompt_action == 3) { // Replace - execute
                    int pos = bm_search(view->rope, 0, app.search_query);
                    int count = 0;
                    while (pos != -1) {
                        /* UNDO DISABLED FOR DEBUG */
                        view->rope = rope_delete(view->rope, pos, strlen(app.search_query));
                        validate_rope(view->rope); // INVARIANT CHECK
                        view->rope = rope_insert(view->rope, pos, app.prompt_buffer);
                        validate_rope(view->rope); // INVARIANT CHECK
                        count++;
                        pos = bm_search(view->rope, pos + (int)strlen(app.prompt_buffer), app.search_query);
                    }
                    snprintf(app.status_msg, sizeof(app.status_msg), "Replaced %d occurrences.", count);
                    app.mode = MODE_NORMAL;
                    
                    // Clamp cursor after mass-replace
                    int total_lines = rope_newlines(view->rope) + 1;
                    if (view->cursor_y >= total_lines) view->cursor_y = total_lines - 1;
                    view_get_position(view); // clamps cursor_x
                    
                    view->modified = 1;
                    view_auto_save(view);
                }
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                if (app.prompt_len > 0) app.prompt_len--;
                app.prompt_buffer[app.prompt_len] = '\0';
            } else if (ch == 27) { // ESC
                app.mode = MODE_NORMAL;
            } else if (ch >= 32 && ch < 127 && app.prompt_len < 255) {
                app.prompt_buffer[app.prompt_len++] = ch;
                app.prompt_buffer[app.prompt_len] = '\0';
            }
        } else {
            app.last_search_pos = -1;
            switch (ch) {
                case KEY_UP: view_move_cursor(view, 0, -1); break;
                case KEY_DOWN: view_move_cursor(view, 0, 1); break;
                case KEY_LEFT: view_move_cursor(view, -1, 0); break;
                case KEY_RIGHT: view_move_cursor(view, 1, 0); break;
                case KEY_BACKSPACE:
                case 127:
                case '\b':
                    view_delete_char(&app, view);
                    break;
                case '\t':
                    if (strlen(app.current_suggestion) > 0) {
                        int prefix_len = strlen(app.current_prefix);
                        int sugg_len = strlen(app.current_suggestion);
                        for (int i = prefix_len; i < sugg_len; i++) {
                            view_insert_char(&app, view, app.current_suggestion[i]);
                        }
                    } else {
                        for (int i = 0; i < 4; i++) view_insert_char(&app, view, ' ');
                    }
                    break;
                case CTRL('w'):
                    app.active_view = (app.active_view + 1) % app.num_views;
                    break;
                case CTRL('f'):
                    app.mode = MODE_PROMPT;
                    strcpy(app.prompt_text, "Find:");
                    app.prompt_len = 0;
                    app.prompt_buffer[0] = '\0';
                    app.prompt_action = 1;
                    break;
                case CTRL('r'):
                    app.mode = MODE_PROMPT;
                    strcpy(app.prompt_text, "Search for (Replace):");
                    app.prompt_len = 0;
                    app.prompt_buffer[0] = '\0';
                    app.prompt_action = 2;
                    break;
                case CTRL('s'):
                    view_save_file(view);
                    snprintf(app.status_msg, sizeof(app.status_msg), "File saved: %s", view->filename);
                    break;
                case CTRL('z'):
                    view_undo(view);
                    break;
                case CTRL('y'):
                    view_redo(view);
                    break;
                case '\n':
                case '\r':
                    view_insert_char(&app, view, '\n');
                    break;
                default:
                    if (ch >= 32 && ch < 127) {
                        view_insert_char(&app, view, ch);
                    }
                    break;
            }
        }
        app_display(&app);
    }

    endwin();
    
    for (int i = 0; i < app.num_views; i++) {
        EditorView *v = &app.views[i];
        if (strlen(v->filename) > 0) {
            char swp_file[300];
            snprintf(swp_file, sizeof(swp_file), ".%s.swp", v->filename);
            remove(swp_file);
        }
        rope_free(v->rope);
        undo_free(&v->undo_stack);
        redo_free(&v->redo_stack);
    }
    trie_free(app.trie);
    hash_free(&app.hash_table);

    return 0;
}
