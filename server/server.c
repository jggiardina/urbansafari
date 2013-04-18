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
#include <errno.h>

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
  Map map;
  Map tempmap;
  //void* player_array[MAXPLAYERS];
  char mapbuf[MAPHEIGHT*MAPWIDTH];
  Player *players[MAXPLAYERS];
  int numplayers;
} globals;

UI *ui;
pthread_mutex_t cur_id_mutex;
int cur_id = 0; //This is used for server_player_init

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

void* server_init_player(int *id, int *team, Tuple *pos)
{
  Player *p = (Player *)malloc(sizeof(Player));
  bzero(p, sizeof(Player));
  //player_init(ui, p);
  
  //Initialize ID and starting conditions
  p->id = cur_id;
  p->state = 0;

  if(p->id % 2 == 0){
    p->team_color = RED;
    p->team = 0;
  }else if(p->id % 2 == 1){
    p->team_color = GREEN;
    p->team = 1;
  }
  
  //Increase the counter for current id's and num players  
  pthread_mutex_lock(&cur_id_mutex);
    cur_id++;
    globals.numplayers++; //Hopefully the mutex on cur_id will suffice
  pthread_mutex_unlock(&cur_id_mutex);

  *id = p->id;
  *team = p->team;
  
  //Find a Home spot to initialize player
  find_free(p->team_color, FLOOR/*HOME*/, &(p->pos));
  ui_uip_init(ui, &p->uip, p->id, p->team);

  pos->x = p->pos.x;
  pos->y = p->pos.y;
  globals.map.cells[p->pos.x + (p->pos.y*globals.map.w)].player = p;
  globals.players[p->id] = p;
  ui_paintmap(ui, &globals.map);
  
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

//TODO: test this code to see if it gets updated, if it doesnt work then change it to only pass s->extra instead.  then there will be warnings with the mutex's tho with &p...cant get to &p.lock its an error
int move(Tuple *pos, void *player){
  int rc = 0;
  Player *p = (Player *)player;
  
  pthread_mutex_lock(&p->lock);
    globals.map.cells[p->pos.x + (p->pos.y*globals.map.w)].player = NULL; // delete player from his old cell
    // TODO: Check if he can make this move:
    p->pos.x += pos->x;
    p->pos.y += pos->y;
    
    globals.map.cells[p->pos.x + (p->pos.y*globals.map.w)].player = p; // add player to his new cell
    
    rc = 1;
    //Return values of player if needing to update
    pos->x = p->pos.x;
    pos->y = p->pos.y;
  
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
        if (p.hammer != NULL){
                proto_session_body_marshall_int(s, 1);
        }else{
                proto_session_body_marshall_int(s, 0);
        }
        if (p.flag == NULL){
                proto_session_body_marshall_int(s, 0);
        }else if (p.flag->c == RED){
                proto_session_body_marshall_int(s, 1);
        }else{
                proto_session_body_marshall_int(s, 2);
        }
  }

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

  pthread_create(&tid, NULL, shell, NULL);
  proto_debug_on();
  // WITH OSX ITS IS EASIEST TO KEEP UI ON MAIN THREAD
  // SO JUMP THROW HOOPS :-(

  /* TESTING LOAD MAP */
  globals.numplayers = 0;
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
        load_map(globals.mapbuf, &globals.map);
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
    if (sym == SDLK_z && mod == KMOD_NONE) return ui_zoom(ui, 1);
    if (sym == SDLK_z && mod & KMOD_SHIFT ) return ui_zoom(ui,-1);
    if (sym == SDLK_LEFT && mod & KMOD_SHIFT) return ui_pan(ui,-1,0);
    if (sym == SDLK_RIGHT && mod & KMOD_SHIFT) return ui_pan(ui,1,0);
    if (sym == SDLK_UP && mod & KMOD_SHIFT) return ui_pan(ui, 0,-1);
    if (sym == SDLK_DOWN && mod & KMOD_SHIFT) return ui_pan(ui, 0,1);
    else {
      fprintf(stderr, "%s: key pressed: %d\n", __func__, sym);
    }
  } else {
    fprintf(stderr, "%s: key released: %d\n", __func__, sym);
  }
  return 1;
}

