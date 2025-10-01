#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <SDL2/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "la.h"
#include "editor.h"

#define FONT_WIDTH 128
#define FONT_HEIGHT 64
#define FONT_COLS 18
#define FONT_ROWS 7
#define FONT_CHAR_WIDTH (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE 2

void scc(int code)
{
    if (code < 0)
    {
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
        exit(1);
    }
}

void *scp(void *ptr)
{
    if (ptr == NULL)
    {
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
        exit(1);
    }
    return ptr;
}

SDL_Surface *surface_from_file(const char *file_path)
{
    int w, h, n;
    unsigned char *pixels = stbi_load(file_path, &w, &h, &n, STBI_rgb_alpha);
    if (pixels == NULL)
    {
        fprintf(stderr, "Failed to load image: %s\n", stbi_failure_reason());
        exit(1);
    }

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int shift = (req_format == STBI_rgb) ? 8 : 0;
    const Uint32 rmask = 0xff000000 >> shift;
    const Uint32 gmask = 0x00ff0000 >> shift;
    const Uint32 bmask = 0x0000ff00 >> shift;
    const Uint32 amask = 0x000000ff >> shift;
#else
    const Uint32 rmask = 0x000000ff;
    const Uint32 gmask = 0x0000ff00;
    const Uint32 bmask = 0x00ff0000;
    const Uint32 amask = 0xff000000;
#endif
    int depth = 32;
    int pitch = w * 4;

    return (SDL_Surface*)scp(SDL_CreateRGBSurfaceFrom(pixels, w, h, depth, pitch, rmask, gmask, bmask, amask));
}

#define ASCII_DISPLAY_LOW 32
#define ASCII_DISPLAY_HIGH 126

typedef struct
{
    SDL_Texture *spritesheet;
    SDL_Rect glyph_table[ASCII_DISPLAY_HIGH - ASCII_DISPLAY_LOW + 1];
} Font;

Font font_load_from_file(SDL_Renderer *renderer, const char *file_path)
{
    Font font = {0};
    SDL_Surface *font_surface = surface_from_file(file_path);
    scc(SDL_SetColorKey(font_surface, SDL_TRUE, 0xFF000000));
    font.spritesheet = (SDL_Texture*)scp(SDL_CreateTextureFromSurface(renderer, font_surface));

    SDL_FreeSurface(font_surface);

    for (size_t ascii = ASCII_DISPLAY_LOW; ascii <= ASCII_DISPLAY_HIGH; ascii++)
    {
        size_t index = ascii - ASCII_DISPLAY_LOW;
        size_t col = index % FONT_COLS;
        size_t row = index / FONT_COLS;

        font.glyph_table[index] = (SDL_Rect){
            .x = (int)(col * FONT_CHAR_WIDTH),
            .y = (int)(row * FONT_CHAR_HEIGHT),
            .w = FONT_CHAR_WIDTH,
            .h = FONT_CHAR_HEIGHT};
    }
    return font;
}

void render_char(SDL_Renderer *renderer, Font *font, char c, Vec2f pos, float scale)
{
    const size_t index = c - ASCII_DISPLAY_LOW;

    const SDL_Rect dst = {
        .x = (int)pos.x,
        .y = (int)pos.y,
        .w = (int)floorf(FONT_CHAR_WIDTH * scale),
        .h = (int)floorf(FONT_CHAR_HEIGHT * scale)};

    assert(c >= ASCII_DISPLAY_LOW && c <= ASCII_DISPLAY_HIGH);
    scc(SDL_RenderCopy(renderer, font->spritesheet, &font->glyph_table[index], &dst));
}

void set_texture_color(SDL_Texture *texture, Uint32 color)
{
    scc(SDL_SetTextureColorMod(texture, (color >> (8 * 0) & 0xff), (color >> (8 * 1) & 0xff), (color >> (8 * 2) & 0xff)));
    SDL_SetTextureAlphaMod(texture, (color >> (8 * 3) & 0xff));
}

void render_text_sized(SDL_Renderer *renderer, Font *font, const char *text, size_t text_size, Vec2f pos, Uint32 color, float scale)
{

    set_texture_color(font->spritesheet, color);

    Vec2f pen = pos;
    for (size_t i = 0; i < text_size; i++)
    {
        render_char(renderer, font, text[i], pen, scale);
        pen = vec2f_add(pen, vec2f(FONT_CHAR_WIDTH * scale, 0));
    }
}

void render_text(SDL_Renderer *renderer, Font *font, const char *text, Vec2f pos, Uint32 color, float scale)
{
    render_text_sized(renderer, font, text, strlen(text), pos, color, scale);
}

#define BUFFER_CAPACITY 1024

Editor editor = {0};

void move_cursor_left()
{
    if (editor.cursor_col > 0)
    {
        editor.cursor_col -= 1;
    }
}

void move_cursor_right()
{
    editor.cursor_col += 1;
}

void move_cursor_up()
{
    if (editor.cursor_row > 0)
    {
        editor.cursor_row -= 1;
    }
}

void move_cursor_down()
{
    editor.cursor_row += 1;
}

#define UNHEX(color) (color) >> (8 * 0) & 0xff, (color >> (8 * 1) & 0xff), (color >> (8 * 2) & 0xff), (color >> (8 * 3) & 0xff)

void render_cursor(SDL_Renderer *renderer, Font *font)
{
    const Vec2f pos = vec2f((float)editor.cursor_col * FONT_CHAR_WIDTH * FONT_SCALE, (float)editor.cursor_row * FONT_CHAR_HEIGHT * FONT_SCALE);

    SDL_Rect rect = {
        .x = (int)floorf(pos.x),
        .y = (int)floorf(pos.y),
        .w = FONT_CHAR_WIDTH * FONT_SCALE,
        .h = FONT_CHAR_HEIGHT * FONT_SCALE};

    scc(SDL_SetRenderDrawColor(renderer, UNHEX(0xffffffff)));
    scc(SDL_RenderFillRect(renderer, &rect));
    const char *c = editor_char_under_cursor(&editor);
    if (c)
    {
        set_texture_color(font->spritesheet, 0xff000000);
        render_char(renderer, font, *c, pos, FONT_SCALE);
    }
}
int main(int argc, char *argv[])
{

    (void)argc;
    (void)argv;

    scc(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window *window = (SDL_Window*)scp(SDL_CreateWindow("Text Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_RESIZABLE));

    SDL_Renderer *renderer = (SDL_Renderer*)scp(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));

    Font font = font_load_from_file(renderer, "./charmap-oldschool_white.png");

    bool quit = false;
    while (!quit)
    {
        SDL_Event event = {0};
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                case SDLK_BACKSPACE:
                    editor_backspace(&editor);
                    break;
                case SDLK_LEFT:
                    move_cursor_left();
                    break;
                case SDLK_RIGHT:
                    move_cursor_right();
                    break;
                case SDLK_DELETE:
                    editor_delete(&editor);
                    break;
                case SDLK_ESCAPE:
                    quit = true;
                    break;
                case SDLK_UP:
                    move_cursor_up(&editor);
                    break;
                case SDLK_DOWN:
                    move_cursor_down(&editor);
                    break;
                case SDLK_RETURN:
                    editor_insert_new_line(&editor);
                    break;
                }
                break;
            case SDL_TEXTINPUT:
                editor_insert_text_before_cursor(&editor, event.text.text);
                // cursor += strlen(event.text.text);
                break;
            }
        }

        scc(SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0));
        scc(SDL_RenderClear(renderer));

        for(size_t row = 0; row < editor.size; ++row){
            const Line *line = &editor.lines[row];
            render_text_sized(renderer, &font, line->chars, line->size, vec2f(0, row * FONT_CHAR_HEIGHT * FONT_SCALE), 0xffffffff, FONT_SCALE);
        }
        render_cursor(renderer, &font);

        SDL_RenderPresent(renderer);
    }

    SDL_Quit();

    return 0;
}