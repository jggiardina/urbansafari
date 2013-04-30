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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <poll.h>
#include "../ui/types.h"
#include "../ui/tty.h"
//#include "../ui/uistandalone.h"
#include "../lib/types.h"
#include "../lib/protocol_server.h"
#include "../lib/protocol_utils.h"
#include "../ui/uistandalone.c"
#define STRLEN 81
#define XSTR(s) STR(s)
#define BUFLEN 16384
#define STR(s) #s

#define MAXPLAYERS 200

struct LineBuffer {
  char data[BUFLEN];
  int  len;
  int  newline;
};

struct Globals {
  struct LineBuffer in;
  int isLoaded;
  pthread_mutex_t MAPLOCK;
  Map map;
  Map tempmap;
  //void* player_array[MAXPLAYERS];
  char mapbuf[MAPHEIGHT*MAPWIDTH];
  Player *players[MAXPLAYERS];
  int numplayers;
  pthread_mutex_t PlayersLock;
  int num_red_players;
  int num_green_players;
} globals;

UI *ui;

/* Find a free spot on a team cell type for a client */
void find_free(Color team_color, Cell_Type cell_type, Pos *p){
  int j, i;
  Cell c;

  for (j = 0; j < MAPHEIGHT; j++){
    for (i = 0; i < MAPWIDTH; i++){
      c = globals.map.cells[i+(j*MAPHEIGHT)];

      if(c.t == cell_type && c.c == team_color){ 
        if(!c.player && !c.hammer && !c.flag){
          p->x = c.p.x;
          p->y = c.p.y;
          i = MAPWIDTH;
          j = MAPHEIGHT;
        }
      }
    }
  }
}

void reset_hammer(int team){
  int x,y;

  if(team == 0){
    globals.map.hammer_1->p.x = 2;
    globals.map.hammer_1->p.y = 90;
    x = globals.map.hammer_1->p.x;
    y = globals.map.hammer_1->p.y;
    globals.map.cells[x+(y*MAPHEIGHT)].hammer = globals.map.hammer_1;
  }else if(team == 1){
    globals.map.hammer_2->p.x = 197;
    globals.map.hammer_2->p.y = 90;
    x = globals.map.hammer_2->p.x;
    y = globals.map.hammer_2->p.y;
    globals.map.cells[x+(y*MAPHEIGHT)].hammer = globals.map.hammer_2;
  }

  //TODO: After reset we must paint the server map, and also send an event to the clients!!! -RC
}

Pos* get_random_team_flag_position(Color team_color){
  int x = rand() % 200;
  int y = rand() % 200;

  Cell c = globals.map.cells[x+(y*MAPHEIGHT)];

  if(c.t == FLOOR && c.player == NULL && c.hammer == NULL && c.c == team_color){
    Pos *p =(Pos *)malloc(sizeof(Pos));
    bzero(p, sizeof(Pos));
    p->x = x;
    p->y = y;
    return p;
  }else{
    return (Pos *)get_random_team_flag_position(team_color);
  }
}

Flag* server_init_flag(Color team_color){
  Flag *flag = (Flag *)malloc(sizeof(Flag));
  bzero(flag, sizeof(Flag));
  Pos *p = get_random_team_flag_position(team_color);
  flag->p.x = p->x;
  flag->p.y = p->y;
  flag->c = team_color;
  flag->discovered = 0;
  
  //TODO: Remove after testing
  //flag->p.x = (int)team_color ? 4 : 3;
  //flag->p.y = (int)team_color ? 90 : 90;

  globals.map.cells[flag->p.x+(flag->p.y*MAPHEIGHT)].flag = flag;
  
  return flag;
}

int remove_player(Player *p, int *numCellsToUpdate, int *cellsToUpdate) {
  pthread_mutex_lock(&globals.PlayersLock);
    pthread_mutex_lock(&globals.MAPLOCK);
      //Drop Flag if they have it
      if(p->flag >= 1)
        drop_flag(&globals.map, p, numCellsToUpdate, cellsToUpdate);
      //Drop Hammer if they have it
      if(p->hammer >= 1)
        drop_hammer(&globals.map, p, numCellsToUpdate, cellsToUpdate);

      // remove player from cell
      globals.map.cells[p->pos.x + (p->pos.y*globals.map.w)].player = NULL;
      cellsToUpdate[*numCellsToUpdate] = (int)&globals.map.cells[p->pos.x + (p->pos.y*globals.map.w)];
      (*numCellsToUpdate)++;
    pthread_mutex_unlock(&globals.MAPLOCK);    
    
    if(p->id % 2 == 0){
      globals.num_red_players--;
    }else if(p->id % 2 == 1){
      globals.num_green_players--;
    }
    globals.players[p->id] = NULL;
    globals.numplayers--; 
  pthread_mutex_unlock(&globals.PlayersLock);

  if(globals.num_red_players == 0 && globals.numplayers > 0){
    proto_server_mt_post_win_handler(1);
  }else if(globals.num_green_players == 0 && globals.numplayers > 0){
    proto_server_mt_post_win_handler(0);
  }
  return 1;
}

void* server_init_player(int *id, int *team, Tuple *pos, int *numCellsToUpdate, int *cellsToUpdate)
{
  pthread_mutex_lock(&globals.PlayersLock);
  
  Player *p = (Player *)malloc(sizeof(Player));
  bzero(p, sizeof(Player));
  pthread_mutex_init(&(p->lock), NULL);
  pthread_mutex_lock(&p->lock);  

  //Initialize ID and starting conditions
  p->id = (int)id;
  p->state = 0;
  
  //Loop through array to find an id
  int i;
  for(i=0;i<=globals.numplayers;i++){
    if(!globals.players[i] && globals.numplayers <= 200){
      p->id = i;
      i = globals.numplayers+1; //break out
    }
  }
  
  if(p->id % 2 == 0){
    p->team_color = RED;
    p->team = 0;
    globals.num_red_players++;
  }else if(p->id % 2 == 1){
    p->team_color = GREEN;
    p->team = 1;
    globals.num_green_players++;
  }
  
  //Increase the counter for num players  
  globals.numplayers++; 

  *id = p->id;
  *team = p->team;
  
  //Find a Home spot to initialize player
  find_free(p->team_color, HOME, &(p->pos));
  ui_uip_init(ui, &p->uip, p->id, p->team);

  pos->x = p->pos.x;
  pos->y = p->pos.y;
  pthread_mutex_lock(&globals.MAPLOCK);
    globals.map.cells[p->pos.x + (p->pos.y*globals.map.w)].player = p;
    cellsToUpdate[*numCellsToUpdate] = (int)&globals.map.cells[p->pos.x + (p->pos.y*globals.map.w)];
      (*numCellsToUpdate)++;
  pthread_mutex_unlock(&globals.MAPLOCK);
  
  globals.players[p->id] = p;
  pthread_mutex_unlock(&p->lock);  

  ui_paintmap(ui, &globals.map);
  
  pthread_mutex_unlock(&globals.PlayersLock);
  return (void *)p;
}

void paint_players(){
  int i;
  SDL_Rect t = {0, 0, ui->tile_h, ui->tile_w};  
  
  for(i=0;i<MAXPLAYERS;i++){
    if(globals.players[i]){
      player_paint(ui, &t, globals.players[i]);
    }
  }
}

/*void add_player(void *p){
  Player *pl = (Player*)p;
  globals.player_array[pl->id] = pl; //Just to put it somewhere for now TODO
}*/

int check_flags_discovered(Pos *p) {
  if (abs(p->x - globals.map.flag_red->p.x) <= 5 && 
      abs(p->y - globals.map.flag_red->p.y) <= 5)
    globals.map.flag_red->discovered = 1;
  if (abs(p->x - globals.map.flag_green->p.x) <= 5 && 
      abs(p->y - globals.map.flag_green->p.y) <= 5)
    globals.map.flag_green->discovered = 1;
}

int move(Tuple *pos, void *player, int *numCellsToUpdate, int *cellsToUpdate){
  int rc = 0;
  Player *p = (Player *)player;
  pthread_mutex_lock(&p->lock);
    pthread_mutex_lock(&globals.MAPLOCK);
      globals.map.cells[p->pos.x + (p->pos.y*MAPWIDTH)].player = NULL; // delete player from his old cell
      cellsToUpdate[*numCellsToUpdate] = (int)&globals.map.cells[p->pos.x + (p->pos.y*MAPWIDTH)];
      (*numCellsToUpdate)++;
      int valid = valid_move(&globals.map, p, pos->x, pos->y, numCellsToUpdate, cellsToUpdate, globals.players, globals.numplayers, globals.num_red_players, globals.num_green_players);
      if (valid == 1){
    	p->pos.x += pos->x;
    	p->pos.y += pos->y;
        // after valid move, check if flags need to be discovered
        if (!globals.map.flag_red->discovered || !globals.map.flag_green->discovered)
          check_flags_discovered(&p->pos);
      }else if(valid == 2){
        proto_server_mt_post_win_handler(0);
      }else if(valid == 3){
        proto_server_mt_post_win_handler(1);
      }
      globals.map.cells[p->pos.x + (p->pos.y*MAPWIDTH)].player = p; // add player to his new cell
      cellsToUpdate[*numCellsToUpdate] = (int)&globals.map.cells[p->pos.x + (p->pos.y*MAPWIDTH)];
      (*numCellsToUpdate)++;
    pthread_mutex_unlock(&globals.MAPLOCK);
    rc = 1;
    //Return values of player if needing to update
    pos->x = p->pos.x;
    pos->y = p->pos.y;

    if(globals.map.cells[p->pos.x+(p->pos.y*MAPHEIGHT)].t == HOME){
      int winner = check_win_condition(&globals.map, globals.numplayers, globals.num_red_players, globals.num_green_players, &globals.players);
      if(winner >= 0)
        proto_server_mt_post_win_handler(winner);
    }
  
  pthread_mutex_unlock(&p->lock);
  ui_paintmap(ui, &globals.map); 
  return rc;
}
int takeHammer(void *player, int *numCellsToUpdate, int *cellsToUpdate){
  int rc = 0;
  Player *p = (Player *)player;

  pthread_mutex_lock(&globals.MAPLOCK);
    pthread_mutex_lock(&p->lock);
      if(take_hammer(&globals.map, p, numCellsToUpdate, cellsToUpdate) == 1){
        rc = 1;
      }else{
	rc = 0;
      }
    pthread_mutex_unlock(&p->lock);
  pthread_mutex_unlock(&globals.MAPLOCK);
  ui_paintmap(ui, &globals.map);
  return rc;
}

int takeFlag(void *player, int *numCellsToUpdate, int *cellsToUpdate){
  int rc = 0;
  Player *p = (Player *)player;
 
  pthread_mutex_lock(&globals.MAPLOCK);
    pthread_mutex_lock(&p->lock);
      if(take_flag(&globals.map, p, numCellsToUpdate, cellsToUpdate) == 1){
        rc = 1;
      }else{
        rc = 0;
      }
    pthread_mutex_unlock(&p->lock);
  pthread_mutex_unlock(&globals.MAPLOCK);
  ui_paintmap(ui, &globals.map);
  return rc;
}

int dropFlag(void *player, int *numCellsToUpdate, int *cellsToUpdate){
  int rc = 0;
  Player *p = (Player *)player;

  pthread_mutex_lock(&p->lock);
  int flag_type = drop_flag(&globals.map, p, numCellsToUpdate, cellsToUpdate);
   if(flag_type > 0){
    rc = flag_type;
    if(globals.map.cells[p->pos.x+(p->pos.y*MAPHEIGHT)].t == HOME){
      int winner = check_win_condition(&globals.map, globals.numplayers, globals.num_red_players, globals.num_green_players, &globals.players);
      if(winner >= 0)
        proto_server_mt_post_win_handler(winner);
    }
   }else{
    rc = 0;
   }
  pthread_mutex_unlock(&p->lock);
  ui_paintmap(ui, &globals.map);
  return rc;
}

int
doUpdateClients(void)
{
  Proto_Session *s;
  Proto_Msg_Hdr hdr;

  s = proto_server_event_session();
  hdr.type = PROTO_MT_EVENT_BASE_UPDATE;
  proto_session_hdr_marshall(s, &hdr);
  proto_server_post_event();  
  return 1;
}

char MenuString[] =
  "server>";


int 
docmd(char cmd)
{
  int rc = 1;

  if (strlen(globals.in.data)==0) rc = 1;
  else if (strncmp(globals.in.data, "load",
                   sizeof("load")-1)==0) rc = doLoad();
  else if (strncmp(globals.in.data, "dump",
                   sizeof("dump")-1)==0) rc = doDump();
  else if (strncmp(globals.in.data, "check",
                   sizeof("check")-1)==0) rc = doCheck();
  else if (strncmp(globals.in.data, "quit",
		sizeof("check")-1)==0) rc = -1;
  return rc;
}
int
doLoad(){
  char filename[256];
  char linebuf[240];
  FILE * myfile;
  int i, n, len;
  sscanf(globals.in.data, "%*s %s", filename);
  //fprintf(stderr, "Opening file %s \n", filename);
  myfile = fopen(filename, "r");
  if ( myfile == NULL ){
    fprintf( stderr, "Could not open file\n" );
  }else{
	n = 0;
        while(fgets(linebuf, sizeof(linebuf), myfile) != NULL){
  		for (i = 0; i < MAPWIDTH; i++){
			globals.mapbuf[i+(n*MAPHEIGHT)] = linebuf[i];
  		}		 
		bzero(linebuf, sizeof(linebuf));
		n++;
	}
	fclose(myfile);
        srand(time(NULL)); //seed random
	//fprintf( stderr, "Read %d lines\n", n);
	load_map(globals.mapbuf, &globals.map);
	globals.isLoaded = 1;
	}
  return 1;
}
int
doDump(){
	if (globals.isLoaded == 1){
		dump_map(&globals.map);
		fprintf(stdout, "%s \n", globals.map.data_ascii);
	}else{
		fprintf(stderr, "no file loaded yet \n", globals.map.data_ascii);
	}
	return 1;
}
int
doCheck(){
	int i;
	doLoad();
	doDump();
	for (i = 0; i < MAPHEIGHT*MAPWIDTH; i++){
		if (globals.mapbuf[i] != globals.map.data_ascii[i]){
			fprintf(stderr, "Not the same.\n");
			return 1;
		}
	}
	fprintf(stdout, "All the same.\n");
	return 1;
}
Map*
getMap(){
  return &globals.map;
}
int
prompt(int menu) 
{
  int ret;
  int c=0;

  if (menu) printf("%s", MenuString);
  fflush(stdout);
  c=getInput();
  return c;
}
char* mapToASCII(){
	dump_map(&globals.map);
	return globals.map.data_ascii;
}
int getAsciiSize(){
	return sizeof(globals.map.data_ascii);
}
char* getPlayers(){
	return globals.players;
}

int marshall_cells_to_update(Proto_Session *s, int *numCellsToUpdate, int *cellsToUpdate){
  Cell **cell_pointers = (Cell **)cellsToUpdate;
  int i;
  Cell c;
  proto_session_body_marshall_int(s, *numCellsToUpdate);
  for (i = 0; i < *numCellsToUpdate; i++){
        c = *(cell_pointers[i]);
        //fprintf(stderr, "id = %d\n", p.id);
        //fprintf(stderr, "x = %d\n", p.pos.x);
        //fprintf(stderr, "y = %d\n", p.pos.y);
        //fprintf(stderr, "team = %d\n", p.team);
        proto_session_body_marshall_int(s, c.p.x);
        proto_session_body_marshall_int(s, c.p.y);
        proto_session_body_marshall_int(s, c.c);
        proto_session_body_marshall_int(s, c.t);
        proto_session_body_marshall_int(s, c.breakable);
  }
}

int marshall_players(Proto_Session *s){
  int i;
  Player p;
  proto_session_body_marshall_int(s, globals.numplayers);
  for (i = 0; i < globals.numplayers; i++){
	p = *(globals.players[i]);
        //fprintf(stderr, "id = %d\n", p.id);
	//fprintf(stderr, "x = %d\n", p.pos.x);
	//fprintf(stderr, "y = %d\n", p.pos.y);
        //fprintf(stderr, "team = %d\n", p.team);
        proto_session_body_marshall_int(s, p.id);
        proto_session_body_marshall_int(s, p.pos.x);
        proto_session_body_marshall_int(s, p.pos.y);
        proto_session_body_marshall_int(s, p.team);
        proto_session_body_marshall_int(s, p.hammer);
        proto_session_body_marshall_int(s, p.flag);
  }
}

int marshall_flags(Proto_Session *s){
  Tuple rflag_pos = {-1, -1};
  // has red been discovered yet?
  proto_session_body_marshall_int(s, globals.map.flag_red->discovered);
  if (globals.map.flag_red->discovered) {
    rflag_pos.x = globals.map.flag_red->p.x;
    rflag_pos.y = globals.map.flag_red->p.y;
  }
  proto_session_body_marshall_int(s, rflag_pos.x);
  proto_session_body_marshall_int(s, rflag_pos.y);
  
  Tuple gflag_pos = {-1, -1};
  // has green been discovered yet?
  proto_session_body_marshall_int(s, globals.map.flag_green->discovered);
  if (globals.map.flag_green->discovered) {
    gflag_pos.x = globals.map.flag_green->p.x;
    gflag_pos.y = globals.map.flag_green->p.y;
  }
  proto_session_body_marshall_int(s, gflag_pos.x);
  proto_session_body_marshall_int(s, gflag_pos.y);
}

int marshall_hammers(Proto_Session *s){
  proto_session_body_marshall_int(s, globals.map.hammer_1->p.x);
  proto_session_body_marshall_int(s, globals.map.hammer_1->p.y);
  proto_session_body_marshall_int(s, globals.map.hammer_2->p.x);
  proto_session_body_marshall_int(s, globals.map.hammer_2->p.y);
}

int
getInput()
{
  int len;
  char *ret;

  // to make debugging easier we zero the data of the buffer
  bzero(globals.in.data, sizeof(globals.in.data));
  globals.in.newline = 0;
  ret = fgets(globals.in.data, sizeof(globals.in.data), stdin);//reads input in from stdin into globals.in.data
  // remove newline if it exists
  len = (ret != NULL) ? strlen(globals.in.data) : 0;//if ret != null, there is a string and thus we set the len to the length of the string, else we set it to 0.
  if (len && globals.in.data[len-1] == '\n') {//if there is a string, and if the last character in said string is '\n', continue
    globals.in.data[len-1]=0;//replace the '\n' with 0;
    globals.in.newline=1;//set the newline property in globals true.
  }
  globals.in.len = len;//set the length property in globals to the determined length.
  return len;
}



void *
shell(void *arg)
{
  int c;
  int rc=1;
  int menu=1;
  globals.isLoaded = 0;

  while (1) {
    if ((c=prompt(menu))!=0) rc=docmd(c);
    if (rc<0) break;
    if (rc==1) menu=1; else menu=0;
  }
  fprintf(stderr, "terminating\n");
  fflush(stdout);
  return NULL;
}

int
main(int argc, char **argv)
{
  pthread_t tid;

  tty_init(STDIN_FILENO);
  
  ui_init(&(ui));
  ui_globals.CELL_W=2; //zoom out right away
  ui_globals.CELL_H=2; //zoom out right away
  ui->tile_h = ui_globals.CELL_H;
  ui->tile_w = ui_globals.CELL_W;
  // center cam on center of map
  Pos center = {MAPWIDTH/2,MAPHEIGHT/2};
  ui_center_cam(ui, &center);

  pthread_create(&tid, NULL, shell, NULL);
  proto_debug_on();
  // WITH OSX ITS IS EASIEST TO KEEP UI ON MAIN THREAD
  // SO JUMP THROW HOOPS :-(
  pthread_mutex_init(&globals.PlayersLock, NULL);
  globals.numplayers = 0;
  pthread_mutex_init(&globals.MAPLOCK, 0);
  globals.num_red_players = 0;
  globals.num_green_players = 0;
  char linebuf[240];
  FILE * myfile;
  int i, n, len;
  myfile = fopen("../server/daGame.map", "r");
  if ( myfile == NULL ){
    fprintf( stderr, "Could not open file\n" );
  }else{
        n = 0;
        while(fgets(linebuf, sizeof(linebuf), myfile) != NULL){
                for (i = 0; i < MAPWIDTH; i++){
                        globals.mapbuf[i+(n*MAPHEIGHT)] = linebuf[i];
                }
                bzero(linebuf, sizeof(linebuf));
                n++;
        }
        fclose(myfile);
        //fprintf( stderr, "Read %d lines\n", n);
        //Initialize the flags and hammers
	globals.map.hammer_1 = (Hammer*)init_hammer();
        globals.map.hammer_2 = (Hammer*)init_hammer();
        load_map(globals.mapbuf, &globals.map);
        globals.map.flag_red = server_init_flag(RED);
        globals.map.flag_green = server_init_flag(GREEN);
        globals.isLoaded = 1;
  }
  if (proto_server_init()<0) {
    fprintf(stderr, "ERROR: failed to initialize proto_server subsystem\n");
    exit(-1);
  }
  fprintf(stdout, "RPC Port: %d, Event Port: %d\n", proto_server_rpcport(),
          proto_server_eventport());

  if (proto_server_start_rpc_loop()<0) {
    fprintf(stderr, "ERROR: failed to start rpc loop\n");
    exit(-1);
  }
  //load_map("../server/daGame.map", &globals.map);
  ui_main_loop(ui, (void *)&globals.map);
  //ui_main_loop(ui, 320, 320);
 
  return 0;
}
extern sval
ui_keypress(UI *ui, SDL_KeyboardEvent *e, void *nothing)
{
  SDLKey sym = e->keysym.sym;
  SDLMod mod = e->keysym.mod;
  
  if (e->type == SDL_KEYDOWN) {
    /*
    if (sym == SDLK_LEFT && mod == KMOD_NONE) {
      fprintf(stderr, "%s: move left\n", __func__);
      return 2;//ui_left(ui);
    }
    if (sym == SDLK_RIGHT && mod == KMOD_NONE) {
      fprintf(stderr, "%s: move right\n", __func__);
      return 2;//ui_right(ui);
    }
    if (sym == SDLK_UP && mod == KMOD_NONE)  {
      fprintf(stderr, "%s: move up\n", __func__);
      return 2;//ui_up(ui);
    }
    if (sym == SDLK_DOWN && mod == KMOD_NONE)  {
      fprintf(stderr, "%s: move down\n", __func__);
      return 2;//ui_down(ui);
    }
    if (sym == SDLK_r && mod == KMOD_NONE)  {
      fprintf(stderr, "%s: dummy pickup red flag\n", __func__);
      return 2;//ui_pickup_red(ui);
    }
    if (sym == SDLK_g && mod == KMOD_NONE)  {
      fprintf(stderr, "%s: dummy pickup green flag\n", __func__);
      return 2;//ui_pickup_green(ui);
    }
    if (sym == SDLK_j && mod == KMOD_NONE)  {
      fprintf(stderr, "%s: dummy jail\n", __func__);
      return 2;//ui_jail(ui);
    }
    if (sym == SDLK_n && mod == KMOD_NONE)  {
      fprintf(stderr, "%s: dummy normal state\n", __func__);
      return 2;//ui_normal(ui);
    }
    if (sym == SDLK_t && mod == KMOD_NONE)  {
      fprintf(stderr, "%s: dummy toggle team\n", __func__);
      return ui_dummy_toggle_team(ui);
    }
    if (sym == SDLK_i && mod == KMOD_NONE)  {
      fprintf(stderr, "%s: dummy inc player id \n", __func__);
      return ui_dummy_inc_id(ui);
    }*/
    if (sym == SDLK_q) return -1;
    if (sym == SDLK_z && mod == KMOD_NONE) {
      int rc = ui_zoom(ui, 1);
      Pos center = {ui_globals.CAMERA_X + (ui_globals.SCREEN_W/ui_globals.CELL_W), ui_globals.CAMERA_Y + (ui_globals.SCREEN_H/ui_globals.CELL_H)};
      //Pos center = {MAPWIDTH/2, MAPHEIGHT/2};
      ui_center_cam(ui, &center);
      return rc;
    }
    if (sym == SDLK_z && mod & KMOD_SHIFT) {
      int rc = ui_zoom(ui, -1);
      Pos center = {ui_globals.CAMERA_X + ((ui_globals.SCREEN_W/ui_globals.CELL_W)/4), ui_globals.CAMERA_Y + ((ui_globals.SCREEN_H/ui_globals.CELL_H)/4)};
      ui_center_cam(ui, &center);
      return rc;
    }
    if (sym == SDLK_LEFT && mod == KMOD_NONE) return ui_pan(ui,-1,0);
    if (sym == SDLK_RIGHT && mod == KMOD_NONE) return ui_pan(ui,1,0);
    if (sym == SDLK_UP && mod == KMOD_NONE) return ui_pan(ui, 0,-1);
    if (sym == SDLK_DOWN && mod == KMOD_NONE) return ui_pan(ui, 0,1);
    else {
      fprintf(stderr, "%s: key pressed: %d\n", __func__, sym);
    }
  } else {
    fprintf(stderr, "%s: key released: %d\n", __func__, sym);
  }
  return 1;
}

