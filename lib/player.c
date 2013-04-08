#include "types.h"
#include "player.h"
//#include "maze.h"

struct {
  pthread_mutex_t lock;
  UI_Player *uip;
  int id;
  int x, y;
  int team;
  int state;
} __attribute__((__packed__)) Player;

static void player_init(UI *ui)
{
  pthread_mutex_init(&(Player.lock), NULL);
  Player.id = 0;
  Player.x = 0; Player.y = 0; Player.team = 0; Player.state = 0;
  ui_uip_init(ui, &Player.uip, Player.id, Player.team);
}

static void player_paint(UI *ui, SDL_Rect *t)
{
  pthread_mutex_lock(&Player.lock);
    t->y = Player.y * t->h; t->x = Player.x * t->w;
    Player.uip->clip.x = Player.uip->base_clip_x +
      pxSpriteOffSet(Player.team, Player.state);
    SDL_BlitSurface(Player.uip->img, &(Player.uip->clip), ui->screen, t);
  pthread_mutex_unlock(&Player.lock);
}

int
ui_left(UI *ui)
{
  pthread_mutex_lock(&Player.lock);
  Player.x--;
  pthread_mutex_unlock(&Player.lock);
  return 2;
}

int
ui_right(UI *ui)
{
  pthread_mutex_lock(&Player.lock);
  Player.x++;
  pthread_mutex_unlock(&Player.lock);
  return 2;
}

int
ui_down(UI *ui)
{
  pthread_mutex_lock(&Player.lock);
  Player.y++;
  pthread_mutex_unlock(&Player.lock);
  return 2;
}

int
ui_up(UI *ui)
{
  pthread_mutex_lock(&Player.lock);
  Player.y--;
  pthread_mutex_unlock(&Player.lock);
  return 2;
}

int
ui_normal(UI *ui)
{
  pthread_mutex_lock(&Player.lock);
  Player.state = 0;
  pthread_mutex_unlock(&Player.lock);
  return 2;
}

int
ui_pickup_red(UI *ui)
{
  pthread_mutex_lock(&Player.lock);
  Player.state = 1;
  pthread_mutex_unlock(&Player.lock);
  return 2;
}

int
ui_pickup_green(UI *ui)
{
  pthread_mutex_lock(&Player.lock);
  Player.state = 2;
  pthread_mutex_unlock(&Player.lock);
  return 2;
}


int
ui_jail(UI *ui)
{
  pthread_mutex_lock(&Player.lock);
  Player.state = 3;
  pthread_mutex_unlock(&Player.lock);
  return 2;
}

int
ui_toggle_team(UI *ui)
{
  pthread_mutex_lock(&Player.lock);
    if (Player.uip) free(Player.uip);
    Player.team = (Player.team) ? 0 : 1;
    ui_uip_init(ui, &Player.uip, Player.id, Player.team);
  pthread_mutex_unlock(&Player.lock);
  return 2;
}

int
ui_inc_id(UI *ui)
{
  pthread_mutex_lock(&Player.lock);
    if (Player.uip) free(Player.uip);
    Player.id++;
    if (Player.id>=100) Player.id = 0;
    ui_uip_init(ui, &Player.uip, Player.id, Player.team);
  pthread_mutex_unlock(&Player.lock);
  return 2;
}
