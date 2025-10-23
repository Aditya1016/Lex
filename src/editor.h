#include <stdlib.h>
#include <stdio.h>

#ifndef EDITOR_H_
#define EDITOR_H_

typedef struct
{
    char *chars;
    size_t capacity;
    size_t size;
} Line;

void line_insert_text_before(Line *line, const char *text, size_t *col);
void line_insert_text_before_sized(Line *line, const char *text, size_t text_size, size_t *col);
void line_backspace(Line *line, size_t *col);
void line_delete(Line *line, size_t *col);
void line_append_text(Line *line, const char* text);
void line_append_text_sized(Line *line, const char* text, size_t text_size);

typedef struct {
    size_t capacity;
    size_t size;
    Line *lines;
    size_t cursor_row;
    size_t cursor_col;
} Editor;

void editor_save_to_file(const Editor *editor, const char* file_path);
void editor_load_from_file(Editor* editor, FILE* file);
void editor_insert_text_before_cursor(Editor *editor, const char *text);
void editor_insert_new_line(Editor *editor);
void editor_backspace(Editor *editor);
void editor_delete(Editor *editor);
const char* editor_char_under_cursor(const Editor* editor);

#endif