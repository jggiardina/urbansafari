#ifndef __DA_GAME_UI_H__
#define __DA_GAME_UI_H__
/******************************************************************************
* Copyright (C) 2011 by Jonathan Appavoo, Boston University
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*****************************************************************************/

#include <SDL/SDL.h>   /* All SDL apps need this */
#include "../lib/maze.h"
#include "types.h"

typedef enum { 
  TEAMA_S=0, TEAMB_S, FLOOR_S, REDWALL_S, GREENWALL_S, LOGO_S, JACKHAMMER_S, REDFLAG_S, GREENFLAG_S, RESULT_S, NUM_S 
} SPRITE_INDEX;

struct UI_Struct {
  SDL_Surface *screen;
  int32_t depth;
  int32_t tile_h;
  int32_t tile_w;

  struct Sprite {
    SDL_Surface *img;
  } sprites[NUM_S];

  uint32_t red_c;
  uint32_t green_c;
  uint32_t blue_c;
  uint32_t white_c;
  uint32_t black_c;
  uint32_t yellow_c;
  uint32_t purple_c;
  uint32_t isle_c;
  uint32_t wall_teama_c;
  uint32_t wall_teamb_c;
  uint32_t player_teama_c;
  uint32_t player_teamb_c;
  uint32_t flag_teama_c;
  uint32_t flag_teamb_c;
  uint32_t jackhammer_c;
};

typedef struct UI_Struct UI;


sval ui_zoom(UI *ui, sval fac);
sval ui_pan(UI *ui, sval xdir, sval ydir);
sval ui_move(UI *ui, sval xdir, sval ydir);
sval ui_keypress(UI *ui, SDL_KeyboardEvent *e, void *C);
void ui_update(UI *ui);
void ui_quit(UI *ui);
//void ui_main_loop(UI *ui, uval h, uval w);
void ui_main_loop(UI *ui, void *map);
extern void ui_init(UI **ui);
extern sval ui_init_sdl(UI *ui, int32_t h, int32_t w, int32_t d);

static void player_init(UI *ui, Player *p);
int ui_left(UI *ui, Player p);
int ui_right(UI *ui, Player p); 
int ui_down(UI *ui, Player p);
int ui_up(UI *ui, Player p);
int ui_normal(UI *ui, Player p);
int ui_pickup_red(UI *ui, Player p);
int ui_pickup_green(UI *ui, Player p);
int ui_jail(UI *ui, Player p);
//int ui_dummy_toggle_team(UI *ui);
//int ui_dummy_inc_id(UI *ui);
extern sval ui_uip_init(UI *ui, UI_Player **p, int id, int team);

#endif
