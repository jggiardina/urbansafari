#include <SDL/SDL.h>
#include "types.h"
#define MAPHEIGHT 200
#define MAPWIDTH 200

typedef enum {RED, GREEN} Color;

typedef enum {JAIL, HOME, FLOOR, WALL} Cell_Type;

typedef struct
{
  int x;
  int y;
} __attribute__((__packed__)) Pos;

typedef struct
{
  Pos p;
  Color c;
} __attribute__((__packed__)) Flag;

typedef struct
{
  Pos p;
  int charges;
} __attribute__((__packed__)) Hammer;

struct UI_Player_Struct {
  SDL_Surface *img;
  uval base_clip_x;
  SDL_Rect clip;
};
typedef struct UI_Player_Struct UI_Player;

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

typedef struct
{
  Pos p;
  Color c;
  Hammer *hammer; //Dont know if we want these
  Flag *flag;  //Dont know if we want these 
  Cell_Type t;
  Player *player; // Pointer to single player that may occupy this cell
} __attribute__((__packed__)) Cell;

typedef struct
{
  char data_ascii[MAPHEIGHT*MAPWIDTH];
  int w;
  int h;
  int num_wall_cells;
  int num_floor_cells;
  Cell cells[sizeof(Cell)*MAPHEIGHT*MAPWIDTH];
} __attribute__((__packed__)) Map;
