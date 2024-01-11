#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include<stdbool.h>
#include<SDL2/SDL_image.h>
#include"la.h"


#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define RED 0
#define GREEN 0
#define BLUE 0
#define ALPHA 0

#define UNHEX(color) \
    ((color) >> (8*2)) & 0xFF, \
    ((color) >> (8*1)) & 0xFF, \
    ((color) >> (8*0)) & 0xFF, \
    ((color) >> (8*3)) & 0xFF \

#define FONT_WIDTH 128
#define FONT_HEIGHT 64
#define FONT_COLS 18
#define FONT_SCALE 3
#define FONT_ROWS 7
#define FONT_CHAR_WIDTH (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)

#define ASCII_START 32
#define ASCII_END 126

#define BUFFER_CAPACITY 1024

struct {
  char buffer[BUFFER_CAPACITY];
  size_t size;
} text;


struct {
  size_t x;
  size_t y;
  bool moved;
} cursor;

typedef struct {
    SDL_Texture *spritesheet;
    SDL_Rect glyph_table[ASCII_END - ASCII_START + 1];
} Font;

typedef struct {
    SDL_Renderer *renderer;
    SDL_Window *window;
} App;

App app;
Vec2f pen;

//Handles error code
void csc(int code){
    if(code < 0){
        fprintf(stderr, "SDL error: %s\n", SDL_GetError());
        exit(1);
    }
}

//Handles if pointer was not assigned
void *csp(void *ptr){
    if(ptr == NULL){
        fprintf(stderr, "SDL error: %s\n", SDL_GetError());
        exit(1);
    }

    return ptr;
}


//Initializing SDL
void initSDL(){
    int rendererFlags, windowFlags;

    rendererFlags = SDL_RENDERER_ACCELERATED;
    windowFlags = SDL_WINDOW_RESIZABLE;

    csc(SDL_Init(SDL_INIT_VIDEO));

    app.window = csp(SDL_CreateWindow("Ion", 0,  0, SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags));
    app.renderer =  csp(SDL_CreateRenderer(app.window,-1,rendererFlags));

}

void doInput(){

        SDL_Event event = {0};

        while (SDL_PollEvent(&event)) {
          switch (event.type) {
              case SDL_QUIT:{
                  exit(0);
                  break;
              }

              case SDL_KEYDOWN: {
                  switch(event.key.keysym.sym){
                      case SDLK_BACKSPACE: {
                          if(text.size > 0){
                              text.size -= 1;
                              cursor.x -= 1;
                          }
                      } break;

                    case SDLK_LEFT:{
                      if(cursor.x > 0){
                        cursor.x -= FONT_CHAR_WIDTH*FONT_SCALE;
                        cursor.moved = true;
                      }
                    }break;

                    case SDLK_RIGHT:{
                      if(cursor.x < pen.x){
                        cursor.x += FONT_CHAR_WIDTH*FONT_SCALE;
                      }else if(cursor.x - 1 == pen.x){
                        cursor.moved = false;
                      }
                    }break;
                    case SDLK_RETURN: {
                      memcpy(text.buffer + text.size, "\n", 1);
                      text.size += 1;
                      cursor.moved = false;
                      cursor.x = 0;
                      cursor.y += FONT_CHAR_HEIGHT * FONT_SCALE;
                    } break;
                  }
              } break;

              case SDL_TEXTINPUT:{
                  size_t len = strlen(event.text.text);
                  const size_t free_space = BUFFER_CAPACITY - text.size;
                  if(len > free_space){
                      len = free_space;
                  }
                  memcpy(text.buffer + text.size, event.text.text, len);
                  text.size += len;
              } break;

              default:
                  break;
          }
        }
}

void prepareScene(){
    csc(SDL_SetRenderDrawColor(app.renderer,RED, GREEN, BLUE,ALPHA));
    csc(SDL_RenderClear(app.renderer));

}

void presentScene(){
    SDL_RenderPresent(app.renderer);
}

//To load an Image
SDL_Surface* initLoadImage(const char* file_path){

    //Initialize the IMG to use PNG format
    int flags = IMG_INIT_PNG;
    int initStatus = IMG_Init(flags);

    if((initStatus & flags) != flags){
        fprintf(stderr, "SDL2_image format not found !!\n");
    }

    SDL_Surface *image = IMG_Load(file_path);

    if(!image){
        fprintf(stderr, "Image not loaded \n");
        exit(1);
    }

    return image;
}



void render_char(Font font, char c, Vec2f pos, float scale){
    //this is where we paint our picture
    const size_t index =  c -  ASCII_START;
    SDL_Rect dest = {
       .x = (int) floorf(pos.x),
       .y = (int) floorf(pos.y),
       .w = (int) floorf(FONT_CHAR_WIDTH * scale),
       .h = (int) floorf(FONT_CHAR_HEIGHT * scale),
    };
    csc(SDL_RenderCopy(app.renderer,font.spritesheet, &font.glyph_table[index], &dest));

}

void render_text(Font font, const char *text, size_t len, Vec2f pos, Uint32 color){

    csc(SDL_SetTextureColorMod(
            font.spritesheet,
            (color >> (8*2)) & 0xff,
            (color >> (8*1)) & 0xff,
            (color >> (8*0)) & 0xff));
    csc(SDL_SetTextureAlphaMod(font.spritesheet,(color >> (8*3)) & 0xff));

    pen = pos;
    for(size_t i = 0; i < len; i++){
      if(text[i] != '\n'){
        render_char(font, text[i], pen,FONT_SCALE);
        pen.x += FONT_CHAR_WIDTH*FONT_SCALE;
      }else{
        pen.x = 0;
        pen.y += FONT_CHAR_HEIGHT*FONT_SCALE;
      }
    }

    if(!cursor.moved){
      cursor.x = pen.x;
      cursor.y = pen.y;
    }


}


Font font_load_from_file(const char *file_path){
    Font font = {0};
    SDL_Surface* font_image = initLoadImage(file_path);
    font.spritesheet = csp(SDL_CreateTextureFromSurface(app.renderer, font_image));
    SDL_FreeSurface(font_image);

    for(size_t ascii = ASCII_START; ascii <= ASCII_END; ascii++){
        const size_t index = ascii - 32;
        const size_t row = index / FONT_COLS;
        const size_t col = index % FONT_COLS;

        // this is from where we take the pixels
        font.glyph_table[index] = (SDL_Rect){
            .x = col * FONT_CHAR_WIDTH,
            .y = row * FONT_CHAR_HEIGHT,
            .w = FONT_CHAR_WIDTH,
            .h = FONT_CHAR_HEIGHT,
        };

    }

    return font;
}

void render_cursor(Uint32 color){
    /* SDL_Rect cursor = { */
    /*    .x = (int)floorf(cursor_x * FONT_CHAR_WIDTH * FONT_SCALE), */
    /*    .y = 0, */
    /*    .w = FONT_CHAR_WIDTH*FONT_SCALE, */
    /*    .h = FONT_CHAR_HEIGHT*FONT_SCALE, */
    /* }; */

    SDL_Rect draw_cursor = {
       .x = cursor.x,
       .y = cursor.y,
       .w = FONT_CHAR_WIDTH*FONT_SCALE,
       .h = FONT_CHAR_HEIGHT*FONT_SCALE,
    };
    csc(SDL_SetRenderDrawColor(app.renderer, UNHEX(color)));

    if(cursor.x < pen.x){
      csc(SDL_RenderDrawRect(app.renderer,&draw_cursor));
    }else{
      csc(SDL_RenderFillRect(app.renderer,&draw_cursor));
    }
}




//TODO: put text_buffer in a structure

int main(){
    initSDL();

    //Taking the image width and height from the surface itself.
    Font font = font_load_from_file("./font.png");

    while(1){
        prepareScene();
        doInput();
        render_text(font, text.buffer, text.size,vec2f(0.0,0.0), 0xFFFFFFFF); //color is in ARGB format
        render_cursor(0xFFFFFFFF);
        presentScene();
        SDL_Delay(20);
    }



    SDL_DestroyTexture(font.spritesheet);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
