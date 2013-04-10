#include "types.h"
#include "player.h"

int cur_id = 0;

static Player player_init(UI *ui)
{
  Player new_player;
  pthread_mutex_init(&(new_player.lock), NULL);
  new_player.id = cur_id;
  
  if (new_player.id>=100){ //increase to 200? -RC
       new_player.id = 0;
    }else if(new_player.id % 2 == 1){
      new_player.team_color = RED;
      new_player.team = 0;
    }else if(new_player.id % 2 == 0){
      new_player.team_color = GREEN;
      new_player.team = 1;
    }

  new_player.x = 0; new_player.y = 0; new_player.state = 0;
  ui_uip_init(ui, &new_player.uip, new_player.id, new_player.team);

  cur_id++;

  return new_player;
}

static void player_paint(UI *ui, SDL_Rect *t, Player player)
{
  pthread_mutex_lock(&player.lock);
    t->y = player.y * t->h; t->x = player.x * t->w;
    player.uip->clip.x = player.uip->base_clip_x +
      pxSpriteOffSet(player.team, player.state);
    SDL_BlitSurface(player.uip->img, &(player.uip->clip), ui->screen, t);
  pthread_mutex_unlock(&player.lock);
}

int ui_left(UI *ui, Player player)
{
  pthread_mutex_lock(&player.lock);
  player.x--;
  pthread_mutex_unlock(&player.lock);
  return 2;
}

int ui_right(UI *ui, Player player)
{
  pthread_mutex_lock(&player.lock);
  player.x++;
  pthread_mutex_unlock(&player.lock);
  return 2;
}

int ui_down(UI *ui, Player player)
{
  pthread_mutex_lock(&player.lock);
  player.y++;
  pthread_mutex_unlock(&player.lock);
  return 2;
}

int ui_up(UI *ui, Player player)
{
  pthread_mutex_lock(&player.lock);
  player.y--;
  pthread_mutex_unlock(&player.lock);
  return 2;
}

int ui_normal(UI *ui, Player player)
{
  pthread_mutex_lock(&player.lock);
  player.state = 0;
  pthread_mutex_unlock(&player.lock);
  return 2;
}

/*int ui_pickup_red(UI *ui, Player player)
{
  pthread_mutex_lock(&player.lock);
  player.state = 1;
  pthread_mutex_unlock(&player.lock);
  return 2;
}*/

/*int ui_pickup_green(UI *ui, Player player)
{
  pthread_mutex_lock(&player.lock);
  player.state = 2;
  pthread_mutex_unlock(&player.lock);
  return 2;
}*/


int ui_jail(UI *ui, Player player)
{
  pthread_mutex_lock(&player.lock);
  player.state = 3;
  pthread_mutex_unlock(&player.lock);
  return 2;
}

int ui_toggle_team(UI *ui, Player player)
{
  pthread_mutex_lock(&player.lock);
    if (player.uip) free(player.uip);
    player.team = (player.team) ? 0 : 1;
    ui_uip_init(ui, &player.uip, player.id, player.team);
  pthread_mutex_unlock(&player.lock);
  return 2;
}

int ui_inc_id(UI *ui)
{
  /*pthread_mutex_lock(&player.lock);
    if (player.uip) free(player.uip);
    player.id++;
    
    if (player.id>=100){ //increase to 200? -RC
       player.id = 0; 
    }
    
    ui_uip_init(ui, &player.uip, player.id, player.team);
  
  pthread_mutex_unlock(&player.lock);
  */
  
  return 2;
}
