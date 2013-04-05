#include <pthread.h>
#include "../ui/uistandalone.c"

typedef struct {
  pthread_mutex_t lock;
  UI_Player *uip;
  int id;
  int x, y;
  int team;
  int state;
} __attribute__((__packed__)) Player;

