#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_ttf.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include<stdbool.h>
#include<SDL2/SDL_image.h>
#include"la.h"


#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define RED 25
#define GREEN 25
#define BLUE 25
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
#define FONT_SIZE 24
#define HORIZONTAL_PADDING 15
#define VERTICAL_PADDING 15

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
  size_t index;
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
bool ctrl_pressed = false;

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

void saveText(){
  const char *file_path = "file.txt";
  FILE *file;

  file = fopen(file_path,"w");

  if(file == NULL){
    fprintf(stderr,"Unable to open file %s\n",file_path);
  }

  fprintf(file,"%s",text.buffer);
  fclose(file);

}

void open_file(const char* filename){
  FILE *file;
  file = fopen(filename, "r");

  if(file == NULL){
    fprintf(stderr, "Unable to open %s\n",filename);
  }

  char ch;
  while ((ch = getc(file)) != EOF) {
    const size_t free_space = BUFFER_CAPACITY - text.size;
    if (free_space > 0) {
      memcpy(text.buffer + text.size, &ch, 1);
      text.size += 1;
    }
  }


  fclose(file);
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
                        if(text.size == 0){
                          break;
                        }
                        if(cursor.moved){
                          memmove(text.buffer+cursor.index,text.buffer+cursor.index+1,text.size);
                          cursor.x -= FONT_CHAR_WIDTH*FONT_SCALE;
                        }
                        cursor.index -=1 ;
                        text.size -=1;
                      } break;

                    case SDLK_LEFT:{
                      if(cursor.x > 0){
                        cursor.x -= FONT_CHAR_WIDTH*FONT_SCALE;
                        cursor.index -= 1;
                        cursor.moved = true;
                      }
                    }break;

                    case SDLK_RIGHT:{
                      if(cursor.x < pen.x){
                        cursor.x += FONT_CHAR_WIDTH*FONT_SCALE;
                        cursor.index += 1;
                      }else if(cursor.x  == pen.x){
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

                    case SDLK_LCTRL:{
                      ctrl_pressed = true;
                    }break;

                    case SDLK_s:{
                      if(ctrl_pressed){
                        saveText();
                        ctrl_pressed = false;
                      }
                    }break;
                  }
              } break;

              case SDL_TEXTINPUT:{
                  const size_t free_space = BUFFER_CAPACITY - text.size;
                  if(free_space == 0){
                    fprintf(stderr,"Full Capacity");
                  }else{
                    if(cursor.moved && cursor.index < text.size - 1){
                      cursor.index += 1;
                      memcpy(text.buffer+cursor.index+1,text.buffer+cursor.index,text.size);
                      memcpy(text.buffer + cursor.index, event.text.text, 1);
                      cursor.x += FONT_CHAR_WIDTH*FONT_SCALE;
                      text.size +=1 ;
                    }else{
                      cursor.moved = false;
                      memcpy(text.buffer + text.size, event.text.text, 1);
                      text.size += 1;
                      cursor.index = text.size - 1;
                    }
                  }
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


TTF_Font* load_font(){

    TTF_Font* font = TTF_OpenFont("/home/iqbal/playground/ion/fonts/OpenSans-Regular.ttf",FONT_SIZE);

  if (!font) {
    printf("TTF_OpenFont: %s\n", TTF_GetError());
  }

  return font;
}



void render_char(TTF_Font* font, char ch, Vec2f pos,int* textWidth, int* textHeight){
    SDL_Color fg = {255, 255, 255, 255};
    SDL_Color bg = {0, 0, 0, 0};
    int minx;
    int maxx;
    int miny;
    int maxy;
    int advance;
    TTF_GlyphMetrics(font,  ch, &minx, &maxx, &miny,&maxy, &advance);
    SDL_Surface* font_surface = TTF_RenderGlyph32_Blended(font,ch,fg);

    SDL_Rect dest = {
       .x = (int) floorf(pos.x),
       .y = (int) floorf(pos.y),
       .w = font_surface->w,
       .h = font_surface->h
    };
    *textWidth = advance;

    SDL_Texture* textureText = SDL_CreateTextureFromSurface(app.renderer, font_surface);

    SDL_FreeSurface(font_surface);

    csc(SDL_RenderCopy(app.renderer,textureText, NULL, &dest));

    SDL_DestroyTexture(textureText);

}

void render_text(TTF_Font* font, const char *text, size_t len, Vec2f pos){


    int textWidth  = 0;
    int textHeight = 0;

    pen = pos;
    for(size_t i = 0; i < len; i++){
      if(text[i] != '\n'){
        render_char(font, text[i], pen,&textWidth,&textHeight);
        pen.x += textWidth;
      }else{
        pen.x = HORIZONTAL_PADDING;
        pen.y += FONT_SIZE + VERTICAL_PADDING;
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

    SDL_Rect draw_cursor = {
       .x = cursor.x,
       .y = cursor.y,
       .w = FONT_SIZE,
       .h = FONT_SIZE,
    };
    csc(SDL_SetRenderDrawColor(app.renderer, UNHEX(color)));

    csc(SDL_RenderFillRect(app.renderer,&draw_cursor));
}

void initTTF(){
    if (TTF_Init() == -1) {
        printf("TTF_Init: %s\n", TTF_GetError());
        exit(2);
    }

}

int main(int argc, char** argv){
    initSDL();
    initTTF();

    TTF_Font* font = load_font();

    if(argc > 1){
      openFile(argv[1]);
    }

    while(1){
        prepareScene();
        doInput();
        render_text(font, text.buffer, text.size,vec2f(HORIZONTAL_PADDING,VERTICAL_PADDING));
        render_cursor(0xFFFFFFFF);
        presentScene();
        SDL_Delay(20);
    }


    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
