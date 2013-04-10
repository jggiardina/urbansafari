#include "../ui/uistandalone.c"
//#include "maze.h"

typedef struct {
  pthread_mutex_t lock;
  UI_Player *uip;
  int id;
  int x, y;
  Color team_color;
  int team;
  Hammer *hammer;
  Flag *flag;
  int state;
} __attribute__((__packed__)) Player;

static Player player_init(UI *ui);
static void player_paint(UI *ui, SDL_Rect *t, Player player);
int ui_left(UI *ui, Player player);
int ui_right(UI *ui, Player player);
int ui_down(UI *ui, Player player);
int ui_up(UI *ui, Player player);
