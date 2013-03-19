#include "net.h"
#include "protocol.h"
#include "protocol_session.h"

typedef enum {JAIL, HOME, FLOOR} Cell_Type;

typedef enum {RED, GREEN} Color;

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

typedef struct
{
  Pos p;
  Hammer h; //Dont know if we want these
  Flag f;  //Dont know if we want these 
  Cell_Type t;
} __attribute__((__packed__)) Cell;

typedef struct
{
  int h;
  int w;
  int num_walls_cells;
  int num_floor_cells;
  Cell cells[];
} __attribute__((__packed__)) Map;
