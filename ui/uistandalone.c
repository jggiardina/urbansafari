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

#include <stdio.h>
#include <stdlib.h> /* for exit() */
#include <pthread.h>
#include <assert.h>
#include "uistandalone.h"
//#include "../lib/maze.h"

/* A lot of this code comes from http://www.libsdl.org/cgi/docwiki.cgi */

/* Forward declaration of some dummy player code */
static void dummyPlayer_init(UI *ui);
static void dummyPlayer_paint(UI *ui, SDL_Rect *t);
static void player_paint(UI *ui, SDL_Rect *t, Player *p);


#define SPRITE_H 32
#define SPRITE_W 32
#define SCREENHEIGHT 320
#define SCREENWIDTH 320

struct UI_Globals {
  int SCREEN_H;
  int SCREEN_W;
  int CELL_H;
  int CELL_W;
  int CAMERA_X;
  int CAMERA_Y;
} ui_globals;

#define UI_FLOOR_BMP "../ui/floor.bmp"
#define UI_REDWALL_BMP "../ui/redwall.bmp"
#define UI_GREENWALL_BMP "../ui/greenwall.bmp"
#define UI_TEAMA_BMP "../ui/teama.bmp"
#define UI_TEAMB_BMP "../ui/teamb.bmp"
#define UI_LOGO_BMP "../ui/logo.bmp"
#define UI_REDFLAG_BMP "../ui/redflag.bmp"
#define UI_GREENFLAG_BMP "../ui/greenflag.bmp"
#define UI_JACKHAMMER_BMP "../ui/shovel.bmp"

typedef enum {UI_SDLEVENT_UPDATE, UI_SDLEVENT_QUIT} UI_SDL_Event;

static inline SDL_Surface *
ui_player_img(UI *ui, int team)
{  
  return (team == 0) ? ui->sprites[TEAMA_S].img 
    : ui->sprites[TEAMB_S].img;
}

static inline sval 
pxSpriteOffSet(int team, int state)
{
  if (state == 1)
    return (team==0) ? SPRITE_W*1 : SPRITE_W*2;
  if (state == 2) 
    return (team==0) ? SPRITE_W*2 : SPRITE_W*1;
  if (state == 3) return SPRITE_W*3;
  return 0;
}

extern sval
ui_uip_init(UI *ui, UI_Player **p, int id, int team)
{
  UI_Player *ui_p;
  
  ui_p = (UI_Player *)malloc(sizeof(UI_Player));
  if (!ui_p) return 0;

  ui_p->img = ui_player_img(ui, team);
  ui_p->clip.w = SPRITE_W; ui_p->clip.h = SPRITE_H; ui_p->clip.y = 0;
  ui_p->base_clip_x = id * SPRITE_W * 4;

  *p = ui_p;

  return 1;
}

/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
static uint32_t 
ui_getpixel(SDL_Surface *surface, int x, int y)
{
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;
  
  switch (bpp) {
  case 1:
    return *p;
  case 2:
    return *(uint16_t *)p;
  case 3:
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
      return p[0] << 16 | p[1] << 8 | p[2];
    else
      return p[0] | p[1] << 8 | p[2] << 16;
  case 4:
    return *(uint32_t *)p;
  default:
    return 0;       /* shouldn't happen, but avoids warnings */
  } // switch
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
static void 
ui_putpixel(SDL_Surface *surface, int x, int y, uint32_t pixel)
 {
   int bpp = surface->format->BytesPerPixel;
   /* Here p is the address to the pixel we want to set */
   uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

   switch (bpp) {
   case 1:
	*p = pixel;
	break;
   case 2:
     *(uint16_t *)p = pixel;
     break;     
   case 3:
     if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
       p[0] = (pixel >> 16) & 0xff;
       p[1] = (pixel >> 8) & 0xff;
       p[2] = pixel & 0xff;
     }
     else {
       p[0] = pixel & 0xff;
       p[1] = (pixel >> 8) & 0xff;
       p[2] = (pixel >> 16) & 0xff;
     }
     break;
 
   case 4:
     *(uint32_t *)p = pixel;
     break;
 
   default:
     break;           /* shouldn't happen, but avoids warnings */
   } // switch
 }

static 
sval splash(UI *ui)
{
  SDL_Rect r;
  SDL_Surface *temp;


  temp = SDL_LoadBMP(UI_LOGO_BMP);
  
  if (temp != NULL) {
    ui->sprites[LOGO_S].img = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    r.h = ui->sprites[LOGO_S].img->h;
    r.w = ui->sprites[LOGO_S].img->w;
    r.x = ui->screen->w/2 - r.w/2;
    r.y = ui->screen->h/2 - r.h/2;
    //    printf("r.h=%d r.w=%d r.x=%d r.y=%d\n", r.h, r.w, r.x, r.y);
    SDL_BlitSurface(ui->sprites[LOGO_S].img, NULL, ui->screen, &r);
  } else {
    /* Map the color yellow to this display (R=0xff, G=0xFF, B=0x00)
       Note:  If the display is palettized, you must set the palette first.
    */
    r.h = 40;
    r.w = 80;
    r.x = ui->screen->w/2 - r.w/2;
    r.y = ui->screen->h/2 - r.h/2;
 
    /* Lock the screen for direct access to the pixels */
    if ( SDL_MUSTLOCK(ui->screen) ) {
      if ( SDL_LockSurface(ui->screen) < 0 ) {
	fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
	return -1;
      }
    }
    SDL_FillRect(ui->screen, &r, ui->yellow_c);

    if ( SDL_MUSTLOCK(ui->screen) ) {
      SDL_UnlockSurface(ui->screen);
    }
  }
  /* Update just the part of the display that we've changed */
  SDL_UpdateRect(ui->screen, r.x, r.y, r.w, r.h);

  SDL_Delay(1000);
  return 1;
}


static sval
load_sprites(UI *ui) 
{
  SDL_Surface *temp;
  sval colorkey;

  /* setup sprite colorkey and turn on RLE */
  // FIXME:  Don't know why colorkey = purple_c; does not work here???
  colorkey = SDL_MapRGB(ui->screen->format, 255, 0, 255);
  
  temp = SDL_LoadBMP(UI_TEAMA_BMP);
  if (temp == NULL) { 
    fprintf(stderr, "ERROR: loading teama.bmp: %s", SDL_GetError()); 
    return -1;
  }
  ui->sprites[TEAMA_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[TEAMA_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
		  colorkey);

  temp = SDL_LoadBMP(UI_TEAMB_BMP);
  if (temp == NULL) { 
    fprintf(stderr, "ERROR: loading teamb.bmp: %s\n", SDL_GetError()); 
    return -1;
  }
  ui->sprites[TEAMB_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);

  SDL_SetColorKey(ui->sprites[TEAMB_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
		  colorkey);
  temp = SDL_LoadBMP(UI_FLOOR_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading floor.bmp %s\n", SDL_GetError()); 
    return -1;
  }
  ui->sprites[FLOOR_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[FLOOR_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
		  colorkey);

  temp = SDL_LoadBMP(UI_REDWALL_BMP);
  if (temp == NULL) { 
    fprintf(stderr, "ERROR: loading redwall.bmp: %s\n", SDL_GetError());
    return -1;
  }
  ui->sprites[REDWALL_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[REDWALL_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
		  colorkey);

  temp = SDL_LoadBMP(UI_GREENWALL_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading greenwall.bmp: %s", SDL_GetError()); 
    return -1;
  }
  ui->sprites[GREENWALL_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[GREENWALL_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		  colorkey);

  temp = SDL_LoadBMP(UI_REDFLAG_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading redflag.bmp: %s", SDL_GetError()); 
    return -1;
  }
  ui->sprites[REDFLAG_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[REDFLAG_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		  colorkey);

  temp = SDL_LoadBMP(UI_GREENFLAG_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading redflag.bmp: %s", SDL_GetError()); 
    return -1;
  }
  ui->sprites[GREENFLAG_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[GREENFLAG_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		  colorkey);

  temp = SDL_LoadBMP(UI_JACKHAMMER_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading %s: %s", UI_JACKHAMMER_BMP, SDL_GetError()); 
    return -1;
  }
  ui->sprites[JACKHAMMER_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[JACKHAMMER_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		  colorkey);
  
  return 1;
}

uint32_t si_to_pix_color_dictionary(UI *ui, SPRITE_INDEX si)
{
  switch (si)
  {
    case 0: // TEAMA_S
      return ui->player_teama_c;
    case 1: // TEAMB_S
      return ui->player_teamb_c;
    case 2: // FLOOR_S
      return ui->isle_c;
    case 3: // REDWALL_S
      return ui->wall_teama_c;
    case 4: // GREENWALL_S
      return ui->wall_teamb_c;
    case 5: // LOGO_S
      return 0; // shouldn't happen
    case 6: // JACKHAMMER_S
      return ui->jackhammer_c;
    case 7: // REDFLAG_S
      return ui->flag_teama_c;
    case 8: // GREENFLAG_S
      return ui->flag_teamb_c;
    default:
      return 0; // shouldn't happen
  }
}

inline static void
draw_cell(UI *ui, SPRITE_INDEX si, SDL_Rect *t, SDL_Surface *s)
{
  SDL_Surface *ts=NULL;
  uint32_t tc;

  ts = ui->sprites[si].img;

  if ( ts && t->h == SPRITE_H && t->w == SPRITE_W) 
    SDL_BlitSurface(ts, NULL, s, t);
  else {// draw just pixels
    int i=0;
    int j=0;
    uint32_t pix = si_to_pix_color_dictionary(ui, si);
    SDL_LockSurface(s);
    for (i = t->x; i < t->x+t->w; i++) {
      for (j = t->y; j < t->y+t->h; j++) {
        ui_putpixel(s, i, j, pix); 
      }
    }
    SDL_UnlockSurface(s);
  }
}

sval
ui_paintmap(UI *ui, Map *map) 
{
  SDL_Rect t;
  int i = ui_globals.CAMERA_X;
  int j = ui_globals.CAMERA_Y;
  t.y = 0; t.x = 0; t.h = ui->tile_h; t.w = ui->tile_w; 

  for (t.y=0; t.y<ui->screen->h; t.y+=t.h) {
    for (t.x=0; t.x<ui->screen->w; t.x+=t.w) {
      if (i >= MAPWIDTH || j >= MAPHEIGHT) {
        draw_cell(ui, FLOOR_S, &t, ui->screen); // paint floor cells outside of the map on the screen
      } else {
        Cell cell = map->cells[i + (j*map->w)];
        if (cell.t == WALL && cell.c == RED) {
          draw_cell(ui, REDWALL_S, &t, ui->screen);
        } else if (cell.t == WALL && cell.c == GREEN) {
          draw_cell(ui, GREENWALL_S, &t, ui->screen);
        } else {
          draw_cell(ui, FLOOR_S, &t, ui->screen);
        }
        // should we also draw a player? - yes
        if (cell.player != NULL) {// for right now, separate for server
          if (t.h == SPRITE_H && t.w == SPRITE_W)
            player_paint(ui, &t, cell.player);//TODO:FIX on the client, we can't use the player pointer so how do we know who to draw?
          else {
            SPRITE_INDEX si = cell.player->team == 0 ? TEAMA_S : TEAMB_S;
            draw_cell(ui, si, &t, ui->screen);
          }
        }
        if(cell.t == FLOOR && cell.hammer){
          draw_cell(ui, JACKHAMMER_S, &t, ui->screen);
        }
        if(cell.flag){
          if(cell.flag->c == RED){
            draw_cell(ui, REDFLAG_S, &t, ui->screen);
          }else{
            draw_cell(ui, GREENFLAG_S, &t, ui->screen);
          }
        }
      }
      i++;
      //draw_cell(ui, FLOOR_S, &t, ui->screen);
    }
    j++;i=ui_globals.CAMERA_X;
  }

  //dummyPlayer_paint(ui, &t);

  SDL_UpdateRect(ui->screen, 0, 0, ui->screen->w, ui->screen->h);
  return 1;
}

sval
ui_paint_y(UI *ui, SDL_Rect *t, int color) {
  SPRITE_INDEX si = color == 0 ? TEAMA_S : TEAMB_S;
  draw_cell(ui, si, t, ui->screen);
  t->x = t->x + 2;
  draw_cell(ui, si, t, ui->screen);
  t->x = t->x - 2; t->y = t->y + 1;
  draw_cell(ui, si, t, ui->screen);
  t->x = t->x + 1;
  draw_cell(ui, si, t, ui->screen); 
  t->x = t->x + 1;
  draw_cell(ui, si, t, ui->screen);
  t->x = t->x - 1; t->y = t->y + 1;
  draw_cell(ui, si, t, ui->screen);
  t->y = t->y + 1;
  draw_cell(ui, si, t, ui->screen);
  return 1;
}


sval
ui_paint_winner(UI *ui, int winner)
{
  SDL_Rect t;
  t.y = 0; t.x = 0; t.h = ui->tile_h; t.w = ui->tile_w;

  for (t.y=0; t.y<ui->screen->h; t.y+=t.h) {
    for (t.x=0; t.x<ui->screen->w; t.x+=t.w) {
      SPRITE_INDEX si = winner == 0 ? TEAMA_S : TEAMB_S;
      draw_cell(ui, si, &t, ui->screen);
    }
  }
  t.x = 20; t.y = 20;
  ui_paint_y(ui, &t, winner);

  SDL_UpdateRect(ui->screen, 0, 0, ui->screen->w, ui->screen->h);
  return 1;
}

sval ui_paint_game_not_started(UI *ui){
  /*SDL_Rect t;
  t.y = 0; t.x = 0; t.h = ui->tile_h; t.w = ui->tile_w;

  for (t.y=0; t.y<ui->screen->h; t.y+=t.h) {
    for (t.x=0; t.x<ui->screen->w; t.x+=t.w) {
      SPRITE_INDEX si = winner == 0 ? TEAMA_S : TEAMB_S;
      draw_cell(ui, si, &t, ui->screen);
    }
  }
  t.x = 20; t.y = 20;
  ui_paint_y(ui, &t, winner);

  SDL_UpdateRect(ui->screen, 0, 0, ui->screen->w, ui->screen->h);
  return 1;*/
}

sval
ui_client_paintmap(UI *ui, Map *map)
{
  SDL_Rect t;
  int i = 0;
  int j = 0;
  t.y = 0; t.x = 0; t.h = ui->tile_h; t.w = ui->tile_w;

  for (t.y=0; t.y<ui->screen->h; t.y+=t.h) {
    for (t.x=0; t.x<ui->screen->w; t.x+=t.w) {
      Cell cell = map->cells[i + (j*map->w)];
      if (cell.t == WALL && cell.c == RED) {
        draw_cell(ui, REDWALL_S, &t, ui->screen);
      } else if (cell.t == WALL && cell.c == GREEN) {
        draw_cell(ui, GREENWALL_S, &t, ui->screen);
      } else {
        draw_cell(ui, FLOOR_S, &t, ui->screen);
      }
      // should we also draw a player?
      if (cell.player != NULL) {// for right now, separate into server and client
        //player_paint(ui, &t, cell.player);//TODO:FIX on the client, we can't use the player pointer so how do we know who to draw?
      }
      i++;
      //draw_cell(ui, FLOOR_S, &t, ui->screen);
    }
    j++;i=0;
  }

  //dummyPlayer_paint(ui, &t);

  SDL_UpdateRect(ui->screen, 0, 0, ui->screen->w, ui->screen->h);
  return 1;
}

static void
player_paint(UI *ui, SDL_Rect *t, Player *p)
{
  pthread_mutex_lock(&(p->lock));
    //t->y = p->pos.y * t->h; t->x = p->pos.x * t->w;//NOTE:We assume all players are drawn in the cell that they come from, so t->x,t->y is already correct
    p->uip->clip.x = p->uip->base_clip_x +
      pxSpriteOffSet(p->team, p->flag);
    SDL_BlitSurface(p->uip->img, &(p->uip->clip), ui->screen, t);
  pthread_mutex_unlock(&(p->lock));
}

extern sval
ui_init_sdl(UI *ui, int32_t h, int32_t w, int32_t d)
{

  fprintf(stderr, "UI_init: Initializing SDL.\n");

  /* Initialize defaults, Video and Audio subsystems */
  if((SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==-1)) { 
    fprintf(stderr, "Could not initialize SDL: %s.\n", SDL_GetError());
    return -1;
  }

  atexit(SDL_Quit);

  fprintf(stderr, "ui_init: h=%d w=%d d=%d\n", h, w, d);

  ui->depth = d;
  ui->screen = SDL_SetVideoMode(w, h, ui->depth, SDL_SWSURFACE);
  if ( ui->screen == NULL ) {
    fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n", w, h, ui->depth, 
	    SDL_GetError());
    return -1;
  }
    
  fprintf(stderr, "UI_init: SDL initialized.\n");


  if (load_sprites(ui)<=0) return -1;

  ui->black_c      = SDL_MapRGB(ui->screen->format, 0x00, 0x00, 0x00);
  ui->white_c      = SDL_MapRGB(ui->screen->format, 0xff, 0xff, 0xff);
  ui->red_c        = SDL_MapRGB(ui->screen->format, 0xff, 0x00, 0x00);
  ui->green_c      = SDL_MapRGB(ui->screen->format, 0x00, 0xff, 0x00);
  ui->yellow_c     = SDL_MapRGB(ui->screen->format, 0xff, 0xff, 0x00);
  ui->purple_c     = SDL_MapRGB(ui->screen->format, 0xff, 0x00, 0xff);

  ui->isle_c         = ui->black_c;
  ui->wall_teama_c   = ui->red_c;
  ui->wall_teamb_c   = ui->green_c;
  ui->player_teama_c = ui->red_c;
  ui->player_teamb_c = ui->green_c;
  ui->flag_teama_c   = ui->white_c;
  ui->flag_teamb_c   = ui->white_c;
  ui->jackhammer_c   = ui->yellow_c;
  
 
  /* set keyboard repeat */
  SDL_EnableKeyRepeat(70, 70);  

  SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
  SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);

  splash(ui);
  return 1;
}

static void
ui_shutdown_sdl(void)
{
  fprintf(stderr, "UI_shutdown: Quitting SDL.\n");
  SDL_Quit();
}

static sval
ui_userevent(UI *ui, SDL_UserEvent *e) 
{
  if (e->code == UI_SDLEVENT_UPDATE) return 2;
  if (e->code == UI_SDLEVENT_QUIT) return -1;
  return 0;
}

static sval
ui_process(UI *ui, Map *map)
{
  SDL_Event e;
  sval rc = 1;

  while(SDL_WaitEvent(&e)) {
    switch (e.type) {
    case SDL_QUIT:
      return -1;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      rc = ui_keypress(ui, &(e.key), NULL);
      break;
    case SDL_ACTIVEEVENT:
      break;
    case SDL_USEREVENT:
      rc = ui_userevent(ui, &(e.user));
      break;
    default:
      fprintf(stderr, "%s: e.type=%d NOT Handled\n", __func__, e.type);
    }
    if (rc==2) { ui_paintmap(ui, map); } 
    if (rc<0) break;
  }
  return rc;
}

static sval
ui_client_process(UI *ui, Map *map, void *C)
{
  SDL_Event e;
  sval rc = 1;

  while(SDL_WaitEvent(&e)) {
    switch (e.type) {
    case SDL_QUIT:
      return -1;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      rc = ui_keypress(ui, &(e.key), C);
      break;
    case SDL_ACTIVEEVENT:
      break;
    case SDL_USEREVENT:
      rc = ui_userevent(ui, &(e.user));
      break;
    default:
      fprintf(stderr, "%s: e.type=%d NOT Handled\n", __func__, e.type);
    }
    if (rc==2) { ui_paintmap(ui, map); } // TODO:FIX Client should only maybe paint player here, not the whole map since it won't have player data
    if (rc<0) break;
  }
  return rc;
}

extern sval
ui_zoom(UI *ui, sval fac)
{
  if (fac == 0) {
    return 1;
  }
  else if (fac > 0 && ui_globals.CELL_H < 32) {
    // zoom in
    ui_globals.CELL_H = ui_globals.CELL_H*2;
    ui_globals.CELL_W = ui_globals.CELL_W*2;
    ui->tile_h = ui_globals.CELL_H;
    ui->tile_w = ui_globals.CELL_W;
  } else if (fac < 0 && ui_globals.CELL_H > 1) {
    // zoom out
    ui_globals.CELL_H = ui_globals.CELL_H/2;
    ui_globals.CELL_W = ui_globals.CELL_W/2;
    ui->tile_h = ui_globals.CELL_H;
    ui->tile_w = ui_globals.CELL_W;
  } else // do nothing
    return 1;
  fprintf(stderr, "%s:\n", __func__);
  return 2;
}

extern sval
ui_pan(UI *ui, sval xdir, sval ydir)
{
  if (ui_globals.CAMERA_X + xdir < 0 || (ui_globals.CAMERA_X + xdir + (ui_globals.SCREEN_W/ui_globals.CELL_W) > MAPWIDTH && xdir == 1))
    return 1;
  if (ui_globals.CAMERA_Y + ydir < 0 || (ui_globals.CAMERA_Y + ydir + (ui_globals.SCREEN_H/ui_globals.CELL_H) > MAPHEIGHT && ydir == 1))
    return 1;
  
  ui_globals.CAMERA_X += xdir;
  ui_globals.CAMERA_Y += ydir;
  fprintf(stderr, "%s:\n", __func__);
  return 2;
}

/* re-wrote this as an RPC
extern sval
ui_move(UI *ui, sval xdir, sval ydir)
{
  fprintf(stderr, "%s:\n", __func__);
  return 1;
}
*/

extern void
ui_update(UI *ui)
{
  SDL_Event event;
  
  event.type      = SDL_USEREVENT;
  event.user.code = UI_SDLEVENT_UPDATE;
  SDL_PushEvent(&event);

}


extern void
ui_quit(UI *ui)
{  
  SDL_Event event;
  fprintf(stderr, "ui_quit: stopping ui...\n");
  event.type      = SDL_USEREVENT;
  event.user.code = UI_SDLEVENT_QUIT;
  SDL_PushEvent(&event);
}

extern void
ui_main_loop(UI *ui, void *m)
{
  Map *map = (Map *)(m);

  uval h = ui_globals.SCREEN_H;
  uval w = ui_globals.SCREEN_W;

  sval rc;
  
  assert(ui);

  ui_init_sdl(ui, h, w, 32);

  //dummyPlayer_init(ui);

  ui_paintmap(ui, map);
   
  
  while (1) {
    if (ui_process(ui, map)<0) break;
  }

  ui_shutdown_sdl();
}


extern void
ui_client_main_loop(UI *ui, void *m, void *C)
{
  Map *map = (Map *)(m);
  /* moved to client main
  uval h = 320;
  uval w = 320;

  sval rc; // dont need

  assert(ui);

  ui_init_sdl(ui, h, w, 32);
  */
  //dummyPlayer_init(ui);

  ui_paintmap(ui, map);

  while (1) {
    if (ui_client_process(ui, map, C)<0) break;
  }

  ui_shutdown_sdl();
}

extern void
ui_init(UI **ui)
{
  *ui = (UI *)malloc(sizeof(UI));
  if (ui==NULL) return;

  bzero(*ui, sizeof(UI));
  
  bzero(&ui_globals, sizeof(ui_globals));

  ui_globals.SCREEN_H = SCREENHEIGHT;
  ui_globals.SCREEN_W = SCREENWIDTH;
  ui_globals.CELL_H = SPRITE_H;
  ui_globals.CELL_W = SPRITE_W;
  ui_globals.CAMERA_X = 0;
  ui_globals.CAMERA_Y = 0;

  (*ui)->tile_h = ui_globals.CELL_H;
  (*ui)->tile_w = ui_globals.CELL_W;

}

extern int
ui_center_cam(UI *ui, Pos *p)
{
  int new_cam_x = p->x - ((ui_globals.SCREEN_W/ui_globals.CELL_W)/2);
  if (new_cam_x <= 0) {
    ui_globals.CAMERA_X = 0;
  } else if (new_cam_x + (ui_globals.SCREEN_W/ui_globals.CELL_W) > MAPWIDTH && ui_globals.CAMERA_X < new_cam_x) {
    ui_globals.CAMERA_X = ui_globals.CAMERA_X; // do nothing
  } else {
    ui_globals.CAMERA_X = new_cam_x;
  }
  int new_cam_y = p->y - ((ui_globals.SCREEN_H/ui_globals.CELL_H)/2);
  if (new_cam_y <= 0) {
    ui_globals.CAMERA_Y = 0;
  } else if (new_cam_y + (ui_globals.SCREEN_H/ui_globals.CELL_H) > MAPHEIGHT && ui_globals.CAMERA_Y < new_cam_y) {
    ui_globals.CAMERA_Y = ui_globals.CAMERA_Y; // do nothing
  } else {
    ui_globals.CAMERA_Y = new_cam_y;
  }
  return 2;
}

int
ui_left(UI *ui, Player p)
{
  pthread_mutex_lock(&p.lock);
    p.pos.x--;
  pthread_mutex_unlock(&p.lock);
  return 2;
}

int
ui_right(UI *ui, Player p)
{
  pthread_mutex_lock(&p.lock);
    p.pos.x++;
  pthread_mutex_unlock(&p.lock);
  return 2;
}

int
ui_down(UI *ui, Player p)
{
  pthread_mutex_lock(&p.lock);
    p.pos.y++;
  pthread_mutex_unlock(&p.lock);
  return 2;
}

int
ui_up(UI *ui, Player p)
{
  pthread_mutex_lock(&p.lock);
    p.pos.y--;
  pthread_mutex_unlock(&p.lock);
  return 2;
}

int
ui_normal(UI *ui, Player p)
{
  pthread_mutex_lock(&p.lock);
    p.state = 0;
  pthread_mutex_unlock(&p.lock);
  return 2;
}

int
ui_pickup_red(UI *ui, Player p)
{
  pthread_mutex_lock(&p.lock);
    p.state = 1;
  pthread_mutex_unlock(&p.lock);
  return 2;
}

int
ui_pickup_green(UI *ui, Player p)
{
  pthread_mutex_lock(&p.lock);
    p.state = 2;
  pthread_mutex_unlock(&p.lock);
  return 2;
}


int
ui_jail(UI *ui, Player p)
{
  pthread_mutex_lock(&p.lock);
    p.state = 3;
  pthread_mutex_unlock(&p.lock);
  return 2;
}
/*
int
ui_dummy_toggle_team(UI *ui)
{
  pthread_mutex_lock(&dummyPlayer.lock);
    if (dummyPlayer.uip) free(dummyPlayer.uip);
    dummyPlayer.team = (dummyPlayer.team) ? 0 : 1;
    ui_uip_init(ui, &dummyPlayer.uip, dummyPlayer.id, dummyPlayer.team);
  pthread_mutex_unlock(&dummyPlayer.lock);
  return 2;
}

int
ui_dummy_inc_id(UI *ui)
{
  pthread_mutex_lock(&dummyPlayer.lock);
    if (dummyPlayer.uip) free(dummyPlayer.uip);
    dummyPlayer.id++;
    if (dummyPlayer.id>=100) dummyPlayer.id = 0;
    ui_uip_init(ui, &dummyPlayer.uip, dummyPlayer.id, dummyPlayer.team);
  pthread_mutex_unlock(&dummyPlayer.lock);
  return 2;
}
*/
