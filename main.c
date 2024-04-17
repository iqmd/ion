#include "la.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define RED 25
#define GREEN 25
#define BLUE 25
#define ALPHA 0

#define UNHEX(color)                                                           \
  ((color) >> (8 * 2)) & 0xFF, ((color) >> (8 * 1)) & 0xFF,                    \
      ((color) >> (8 * 0)) & 0xFF, ((color) >> (8 * 3)) & 0xFF

#define FONT_SIZE 20
#define HORIZONTAL_PADDING 15
#define VERTICAL_PADDING FONT_SIZE * 0.20

#define BUFFER_CAPACITY 1024 * 1000

typedef struct {
  char buffer[BUFFER_CAPACITY];
  size_t size;
} Text;

typedef struct {
  size_t x;
  size_t y;
  size_t index;
  bool moved;
} Cursor;

typedef struct {
  SDL_Renderer *renderer;
  SDL_Window *window;
} App;

typedef struct {
  App app;
  Vec2f pen;
  bool ctrl_pressed;
  Text text;
  Cursor cursor;
  size_t text_width;
  size_t char_width;
  size_t char_height;
  int row;
  int col;

} Editor;

Editor E;

// Handles error code
void csc(int code) {
  if (code < 0) {
    fprintf(stderr, "SDL error: %s\n", SDL_GetError());
    exit(1);
  }
}

// Handles if pointer was not assigned
void *csp(void *ptr) {
  if (ptr == NULL) {
    fprintf(stderr, "SDL error: %s\n", SDL_GetError());
    exit(1);
  }

  return ptr;
}

// Initializing SDL
void initSDL() {
  int rendererFlags, windowFlags;

  rendererFlags = SDL_RENDERER_ACCELERATED;
  windowFlags = SDL_WINDOW_RESIZABLE;

  csc(SDL_Init(SDL_INIT_VIDEO));

  E.app.window = csp(
      SDL_CreateWindow("Ion", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags));
  E.app.renderer = csp(SDL_CreateRenderer(E.app.window, -1, rendererFlags));
}

void saveText() {
  const char *file_path = "file.txt";
  FILE *file;

  file = fopen(file_path, "w");

  if (file == NULL) {
    fprintf(stderr, "Unable to open file %s\n", file_path);
  }

  fprintf(file, "%s", E.text.buffer);
  fclose(file);
}

void open_file(const char *filename) {
  FILE *file;
  file = fopen(filename, "r");

  if (file == NULL) {
    fprintf(stderr, "Unable to open %s\n", filename);
  }

  char ch;
  while ((ch = getc(file)) != EOF) {
    const size_t free_space = BUFFER_CAPACITY - E.text.size;
    if (free_space > 0) {
      memcpy(E.text.buffer + E.text.size, &ch, 1);
      E.text.size += 1;
    }
  }

  fclose(file);
}

void doInput() {

  SDL_Event event = {0};

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT: {
      exit(0);
      break;
    }

    case SDL_KEYDOWN: {
      switch (event.key.keysym.sym) {
      case SDLK_BACKSPACE: {
        if (E.text.size == 0) {
          break;
        }
        if (E.cursor.moved) {
          memmove(E.text.buffer + E.cursor.index,
                  E.text.buffer + E.cursor.index + 1, E.text.size);
          E.cursor.x -= E.char_width;
        }
        E.cursor.index -= 1;
        E.text.size -= 1;
        E.col -= 1;
      } break;

      case SDLK_LEFT: {
        if (E.cursor.x > 0) {
          E.cursor.x -= E.char_width;
          E.cursor.index -= 1;
          E.cursor.moved = true;
          E.col -= 1;
        }
      } break;

      case SDLK_RIGHT: {
        if (E.cursor.x < E.pen.x) {
          E.cursor.x += E.char_width;
          E.cursor.index += 1;
          E.col += 1;
        } else if (E.cursor.x == E.pen.x) {
          E.cursor.moved = false;
        }
      } break;
      case SDLK_RETURN: {
        memcpy(E.text.buffer + E.text.size, "\n", 1);
        E.text.size += 1;
        E.cursor.moved = false;
        E.cursor.x = 0;
        E.cursor.y += E.char_height;
        E.row += 1;
      } break;

      case SDLK_LCTRL: {
        E.ctrl_pressed = true;
      } break;

      case SDLK_s: {
        if (E.ctrl_pressed) {
          saveText();
          E.ctrl_pressed = false;
        }
      } break;
      }
    } break;

    case SDL_TEXTINPUT: {
      const size_t free_space = BUFFER_CAPACITY - E.text.size;
      if (free_space == 0) {
        fprintf(stderr, "Full Capacity");
      } else {
        if (E.cursor.moved && E.cursor.index < E.text.size - 1) {
          E.cursor.index += 1;
          memcpy(E.text.buffer + E.cursor.index + 1,
                 E.text.buffer + E.cursor.index, E.text.size);
          memcpy(E.text.buffer + E.cursor.index, event.text.text, 1);
          E.cursor.x += E.char_width;
          E.text.size += 1;
          E.col += 1;
        } else {
          E.cursor.moved = false;
          memcpy(E.text.buffer + E.text.size, event.text.text, 1);
          E.text.size += 1;
          E.col += 1;
          E.cursor.index = E.text.size - 1;
        }
      }
    } break;

    default:
      break;
    }
  }
}

void prepareScene() {
  csc(SDL_SetRenderDrawColor(E.app.renderer, RED, GREEN, BLUE, ALPHA));
  csc(SDL_RenderClear(E.app.renderer));
}

void presentScene() { SDL_RenderPresent(E.app.renderer); }

TTF_Font *load_font() {

  TTF_Font *font = TTF_OpenFont("/home/iqbal/playground/ion/fonts/Hack Regular "
                                "Nerd Font Complete Mono.ttf",
                                FONT_SIZE);
  E.char_height = TTF_FontHeight(font);
  E.char_width = TTF_FontFaceIsFixedWidth(font);

  if (!font) {
    printf("TTF_OpenFont: %s\n", TTF_GetError());
  }

  return font;
}

void render_char(TTF_Font *font, char ch, Vec2f pos) {
  SDL_Color fg = {255, 255, 255, 255};

  SDL_Surface *font_surface = TTF_RenderGlyph32_Blended(font, ch, fg);

  SDL_Rect dest = {.x = (int)floorf(pos.x),
                   .y = (int)floorf(pos.y),
                   .w = font_surface->w,
                   .h = font_surface->h};

  E.char_width = font_surface->w;
  E.char_height = font_surface->h;

  SDL_Texture *textureText =
      SDL_CreateTextureFromSurface(E.app.renderer, font_surface);

  SDL_FreeSurface(font_surface);

  csc(SDL_RenderCopy(E.app.renderer, textureText, NULL, &dest));

  SDL_DestroyTexture(textureText);
}

void render_text(TTF_Font *font, const char *text, size_t len, Vec2f pos) {

  E.pen = pos;
  for (size_t i = 0; i < len; i++) {
    if (text[i] != '\n') {
      render_char(font, text[i], E.pen);
      E.pen.x += E.char_width;
    } else {
      E.pen.x = 0;
      E.pen.y += E.char_height + VERTICAL_PADDING;
    }
  }

  if (!E.cursor.moved) {
    E.cursor.x = E.pen.x;
    E.cursor.y = E.pen.y;
  }
}

void render_cursor(Uint32 color) {

  SDL_Rect draw_cursor = {
      .x = E.cursor.x,
      .y = E.cursor.y,
      .w = E.char_width * 0.15,
      .h = E.char_height,
  };
  csc(SDL_SetRenderDrawColor(E.app.renderer, UNHEX(color)));

  csc(SDL_RenderFillRect(E.app.renderer, &draw_cursor));
}

void initTTF() {
  if (TTF_Init() == -1) {
    printf("TTF_Init: %s\n", TTF_GetError());
    exit(2);
  }
}

int main(int argc, char **argv) {
  initSDL();
  initTTF();

  TTF_Font *font = load_font();

  if (argc > 1) {
    open_file(argv[1]);
  }

  while (1) {
    prepareScene();
    doInput();
    render_text(font, E.text.buffer, E.text.size, vec2f(0.0, 0.0));
    render_cursor(0xFFFFFFFF);
    presentScene();
    SDL_Delay(20);
  }

  TTF_CloseFont(font);
  TTF_Quit();
  SDL_Quit();

  return 0;
}
