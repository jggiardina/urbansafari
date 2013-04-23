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

#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"
#include "../ui/tty.h"
#include "../ui/uistandalone.c"

#define STRLEN 81
#define XSTR(s) STR(s)
#define BUFLEN 16384
#define STR(s) #s

struct LineBuffer {
  char data[BUFLEN];
  int  len;
  int  newline;
};

struct Globals {
  struct LineBuffer in;
  char host[STRLEN];
  PortType port;
  int connected;
  // KLUDGY GLOBALS FOR MAP LOAD:
  int isLoaded;
  Map map;
  char mapbuf[MAPHEIGHT*MAPWIDTH];
  Player players[200];
} globals;

typedef struct ClientState  {
  void *data;
  Proto_Client_Handle ph;
} Client;

UI *ui;

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


int
prompt(int menu) 
{
  static char MenuString[] = "\nclient> ";
  int ret;
  int c=0;

  if (menu) printf("%s", MenuString);
  fflush(stdout);
  if (globals.connected) {
    c = getchar();
    return c;
  } else {
    int len = getInput();
    return (len) ? 1 : -1;    
  }
}

int do_move_left_rpc(UI *ui, Client *C)
{
  int rc=0;
  Tuple tuple = {-1, 0};
  rc = proto_client_move(C->ph, &tuple); // left
  Player *p = (Player *)C->data;
  pthread_mutex_lock(&p->lock);
    p->pos.x = tuple.x;
    p->pos.y = tuple.y;
    ui_center_cam(ui, &p->pos);
  pthread_mutex_unlock(&p->lock);
  return rc;
}

int do_move_right_rpc(UI *ui, Client *C)
{
  int rc=0;
  Tuple tuple = {1, 0};
  rc = proto_client_move(C->ph, &tuple); // right
  Player *p = (Player *)C->data;
  pthread_mutex_lock(&p->lock);
    p->pos.x = tuple.x;
    p->pos.y = tuple.y;
    ui_center_cam(ui, &p->pos);
  pthread_mutex_unlock(&p->lock);
  return rc;
}

int do_move_up_rpc(UI *ui, Client *C)
{
  int rc=0;
  Tuple tuple = {0, -1};
  rc = proto_client_move(C->ph, &tuple); // up
  Player *p = (Player *)C->data;
  pthread_mutex_lock(&p->lock);
    p->pos.x = tuple.x;
    p->pos.y = tuple.y;
    ui_center_cam(ui, &p->pos);
  pthread_mutex_unlock(&p->lock);
  return rc;
}

int do_move_down_rpc(UI *ui, Client *C)
{
  int rc=0;
  Tuple tuple = {0, 1};
  rc = proto_client_move(C->ph, &tuple); // down
  Player *p = (Player *)C->data;
  pthread_mutex_lock(&p->lock);
    p->pos.x = tuple.x;
    p->pos.y = tuple.y;
    ui_center_cam(ui, &p->pos);
  pthread_mutex_unlock(&p->lock);
  return rc;
}

int do_pickup_flag_rpc(UI *ui, Client *C)
{
  int flag = 0;
  proto_client_pick_up_flag(C->ph, &flag);
  Player *p = (Player *)C->data;
  pthread_mutex_lock(&p->lock);
    p->flag = flag;
  pthread_mutex_unlock(&p->lock);
}

int do_pickup_hammer_rpc(UI *ui, Client *C)
{
  int hammer = 0;
  proto_client_pick_up_hammer(C->ph, &hammer);
  Player *p = (Player *)C->data;
  pthread_mutex_lock(&p->lock);
    p->hammer = hammer;
  pthread_mutex_unlock(&p->lock);
}

int 
doRPCCmd(UI *ui, Client *C, char c) 
{
  int rc=-1;
  if (c != 'm') { return 1;}
  c = getchar();
  fprintf(stderr, "\n\nthis should be 0-3: %c\n", c);
  switch (c) {
  case '0':
    //printf("w ->do rpc: up\n");
    do_move_up_rpc(ui, C);
    rc=2;
    break;
  case '1':
    //printf("s ->do rpc: down\n");
    do_move_down_rpc(ui, C);
    rc=2;
    break;
  case '2':
    //printf("a ->do rpc: left\n");
    do_move_left_rpc(ui, C);
    rc=2;
    break;
  case '3':
    //printf("d ->do rpc: right\n");
    do_move_right_rpc(ui, C);
    rc=2;
    break;
  default:
    return 1;
  }
  if (rc==2) ui_update(ui);
  return rc;
}

int
doRPC(UI *ui, Client *C)
{
  fprintf(stderr, "got into r\n");
  int rc;
  char c;
  int d;
  //printf("enter (h|m<c>|g): ");
  c = getchar();//scanf("%c", &c);
  fprintf(stderr, "\n\nthis should be m: %c\n", c);
  rc=doRPCCmd(ui,C,c);
  //d=(int)getchar();
  //fprintf(stderr, "\n\nthis should be 0-4: %d\n", d);
  //sleep(d);// TODO: ONLY FOR COMMAND LINE TESTING
  //printf("doRPC: rc=0x%x\n", rc);

  return rc;
}

int 
docmd(Client *C, char cmd)
{
  int rc = 1;
  // not yet connected so don't use the single char command prompt
  /*if (!globals.connected) {
    if (strlen(globals.in.data)==0) rc = doEnter(C);
    else if (strncmp(globals.in.data, "connect",
                   sizeof("connect")-1)==0) rc = doConnect(C);
    else if (strncmp(globals.in.data, "disconnect",
                   sizeof("disconnect")-1)==0) rc = doDisconnect(C);
    else if (strncmp(globals.in.data, "quit",
                   sizeof("quit")-1)==0) rc = doQuit(C);
    //else if (strncmp(globals.in.data, "numhome",
    //               sizeof("numhome")-1)==0) rc = doMapInfoTeam(C, 'h');
    //else if (strncmp(globals.in.data, "numjail",
    //               sizeof("numjail")-1)==0) rc = doMapInfoTeam(C, 'j');
    //else if (strncmp(globals.in.data, "numwall",
    //               sizeof("numwall")-1)==0) rc = doMapInfo(C, 'w');
    //else if (strncmp(globals.in.data, "numfloor",
    //               sizeof("numfloor")-1)==0) rc = doMapInfo(C, 'f');
    //else if (strncmp(globals.in.data, "dump",
    //               sizeof("dump")-1)==0) rc = doMapDump(C);
    //else if (strncmp(globals.in.data, "dim",
    //               sizeof("dim")-1)==0) rc = doMapDim(C);
    //else if (strncmp(globals.in.data, "cinfo",
    //              sizeof("cinfo")-1)==0) rc = doMapCinfo(C);
    else {
      fprintf(stderr, "Invalid command\n");
      rc = 1;
    }
    return rc;
  }
  */
  // otherwise do the tty commands
  fprintf(stderr, "\n\nthis should be r: %c\n", cmd);
  switch (cmd) {
  case 'q':
    printf("q ->quitting...\n");
    doQuit();
    rc=-1;
    break;
  case 'r':
    rc=doRPC(ui, C);
    break;
  case 'w':
    printf("w ->do rpc: up\n");
    do_move_up_rpc(ui, C);
    rc=2;
    break;
  case 's':
    printf("s ->do rpc: down\n");
    do_move_down_rpc(ui, C);
    rc=2;
    break;
  case 'a':
    printf("a ->do rpc: left\n");
    do_move_left_rpc(ui, C);
    rc=2;
    break;
  case 'd':
    printf("d ->do rpc: right\n");
    do_move_right_rpc(ui, C);
    rc=2;
    break;
  case '\n':
    rc=1;
    break;
  default:
    printf("Unkown Command\n");
  }
  if (rc==2) ui_update(ui);
  return rc;
}

void *
shell(void *arg)
{
  Client *C = arg;
  char c;
  int rc;
  int menu=1;

  pthread_detach(pthread_self());

  while (1) {
    if ((c=prompt(menu))!=0) rc=docmd(C, c);/*not sure about this*/else rc = -1;
    if (rc<0) break;
    if (rc==1) menu=1; else menu=0;
  }

  fprintf(stderr, "terminating\n");
  fflush(stdout);
  ui_quit(ui);
  return NULL;
}
char*
getMapPointer(){
        return &globals.map;
}
char*
getMapBufPointer(){
        return globals.mapbuf;
}
int
getMapSize(){
        return sizeof(globals.mapbuf);
}
void
convertMap(){
        load_map(globals.mapbuf, &globals.map);
}

// old client event handlers:
static int
update_event_handler(Proto_Session *s)
{

  int numplayers, i, offset;
  Client *C = proto_session_get_data(s);
  Player *me = (Player *)C->data;

  proto_session_body_unmarshall_bytes(s, 0, getMapSize(), getMapBufPointer());
  convertMap();

  offset = getMapSize();

  //Unmarshall Players
  proto_session_body_unmarshall_int(s, offset, &numplayers);
  //fprintf(stderr, "num players = %d\n", numplayers);
  bzero(globals.players, numplayers*sizeof(Player));
  offset+= sizeof(int);
  for (i = 0; i < numplayers; i++){
    pthread_mutex_init(&(globals.players[i].lock), NULL);
    //pthread_mutex_lock(&cur_id_mutex);
    proto_session_body_unmarshall_int(s, offset, &(globals.players[i].id));
    proto_session_body_unmarshall_int(s, offset+sizeof(int), &(globals.players[i].pos.x));
    //fprintf(stderr, "x = %d\n", players[i].pos.x);
    proto_session_body_unmarshall_int(s, offset + 2*sizeof(int), &(globals.players[i].pos.y));
    //fprintf(stderr, "x = %d\n", players[i].pos.y);
    proto_session_body_unmarshall_int(s, offset + 3*sizeof(int), &(globals.players[i].team));
    //fprintf(stderr, "team = %d\n", players[i].team);
    proto_session_body_unmarshall_int(s, offset + 4*sizeof(int), &(globals.players[i].hammer));
    //fprintf(stderr, "hammer = %d\n", players[i].hammer);
    proto_session_body_unmarshall_int(s, offset + 5*sizeof(int), &(globals.players[i].flag));
    //fprintf(stderr, "flag = %d\n", players[i].flag);
    offset+= 6*sizeof(int);
    globals.map.cells[globals.players[i].pos.x + (globals.players[i].pos.y*MAPHEIGHT)].player = &(globals.players[i]);
    //pthread_mutex_unlock(&cur_id_mutex);
    //STATE
    ui_uip_init(ui, &globals.players[i].uip, globals.players[i].id, globals.players[i].team);      
    
    // update me
    if (globals.players[i].id == me->id) {
      pthread_mutex_lock(&me->lock);
        me->pos.x = globals.players[i].pos.x;
        me->pos.y = globals.players[i].pos.y;
        me->team = globals.players[i].team;
        me->flag = globals.players[i].flag;
        me->hammer = globals.players[i].hammer;
        //ui_center_cam(ui, &me->pos);
      pthread_mutex_unlock(&me->lock);
    }
  }

  //Unmarshall Flags
  proto_session_body_unmarshall_int(s, offset, &(globals.map.flag_red->p.x));
  proto_session_body_unmarshall_int(s, offset+sizeof(int), &(globals.map.flag_red->p.y));
  proto_session_body_unmarshall_int(s, offset+2*sizeof(int), &(globals.map.flag_green->p.x));
  proto_session_body_unmarshall_int(s, offset+3*sizeof(int), &(globals.map.flag_green->p.y));

  int x,y;
  //Red Flag
  x = globals.map.flag_red->p.x;
  y = globals.map.flag_red->p.y;
  globals.map.cells[x+(y*MAPHEIGHT)].flag = globals.map.flag_red;
  //Green Flag
  x = globals.map.flag_green->p.x;
  y = globals.map.flag_green->p.y;
  globals.map.cells[x+(y*MAPHEIGHT)].flag = globals.map.flag_green;
 
  ui_paintmap(ui, &globals.map);//TODO: this call is making the movement a little laggy - need to optimize this function so we paint quicker 

  fprintf(stderr, "%s: called", __func__);
  return 1;
}

static int
hello_event_handler(Proto_Session *s)
{
  Client *C = proto_session_get_data(s);
  
  fprintf(stderr, "%s: called", __func__);
  return 1;
}

static int
goodbye_event_handler(Proto_Session *s)
{
  Client *C = proto_session_get_data(s);

  fprintf(stderr, "%s: called", __func__);
  return 1;
}

void
usage(char *pgm)
{
  fprintf(stderr, "USAGE: %s <port|<<host port> [shell] [gui]>>\n"
  " port : rpc port of a game server if this is only argument\n"
  " specified then host will default to localhost and\n"
  " only the graphical user interface will be started\n"
  " host port: if both host and port are specifed then the game\n"
  "examples:\n"
  " %s 12345 : starts client connecting to localhost:12345\n"
  " %s localhost 12345 : starts client connecting to locaalhost:12345\n",
  pgm, pgm, pgm, pgm);
}

void
initGlobals(int argc, char **argv)
{
  if (argc==1) {
    usage(argv[0]);
    exit(-1);
  }

  if (argc==2) {
    strncpy(globals.host, "localhost", STRLEN);
    globals.port = atoi(argv[1]);
  }

  if (argc>=3) {
    strncpy(globals.host, argv[1], STRLEN);
    globals.port = atoi(argv[2]);
  }

}
void
initMap(Map *m, int size){
	if (size == sizeof(globals.map)){
		memcpy(&globals.map, m, size);
  		ui_paintmap(ui, &globals.map);
	 }else{
		fprintf(stderr, "ERROR: size of recieved map invalid\n");
	}

}

Flag* client_init_flag(Color team_color){
  Flag *flag = (Flag *)malloc(sizeof(Flag));
  bzero(flag, sizeof(Flag));
  Pos p = {0,0};
  flag->p.x = p.x;
  flag->p.y = p.y;
  flag->c = team_color;
  //globals.map.cells[p->x+(p->y*MAPHEIGHT)].flag = flag;

  return flag;
}

int
main(int argc, char **argv)
{
  // ORIGINAL CLIENT
  Client c;
  bzero(&globals, sizeof(globals));
  initGlobals(argc, argv);
  if (clientInit(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }
  // END ORIGINAL CLIENT
  
  // init ui code
  tty_init(STDIN_FILENO);
  ui_init(&(ui));
  assert(ui);
  ui_init_sdl(ui, ui_globals.SCREEN_H, ui_globals.SCREEN_W, 32);
  // end init ui code

  // RUN AUTO-CONNECT FIRST
  if (startConnection(&c, globals.host, globals.port, update_event_handler)<0) return -1;
  else {
    globals.connected = 1;
    fprintf(stdout, "Connected to <%s:%d>\n", globals.host, globals.port); 
  }
  // center the camera
  Player *p = (Player *)c.data;
  ui_center_cam(ui, &p->pos);
  // END CONNECT

  // Initialize the hammers
  globals.map.hammer_1 = (Hammer*)init_hammer();
  globals.map.hammer_2 = (Hammer*)init_hammer();
  
  // Initialize the flags
  globals.map.flag_red = (Flag*)client_init_flag(RED);
  globals.map.flag_green = (Flag*)client_init_flag(GREEN); 
 
  pthread_t tid;
  pthread_create(&tid, NULL, shell, &c);

  // WITH OSX ITS IS EASIEST TO KEEP UI ON MAIN THREAD
  // SO JUMP THROW HOOPS :-(
  
  proto_debug_on();
  ui_client_main_loop(ui, (void *)&globals.map, &c); //TODO:FIX if the update_event_handler for hello is not hit before this, then the map will not be initialized and the main loop will just print all floor (JAIL) cells until the handler is hit.
  return 0;
}

extern sval
ui_keypress(UI *ui, SDL_KeyboardEvent *e, void *client)
{
  SDLKey sym = e->keysym.sym;
  SDLMod mod = e->keysym.mod;

  Client *C = (Client *)client;
  if (e->type == SDL_KEYDOWN) {
    if (sym == SDLK_LEFT && mod == KMOD_NONE) {
      fprintf(stderr, "%s: move left\n", __func__);
      do_move_left_rpc(ui, C);
      return 2;
    }
    if (sym == SDLK_RIGHT && mod == KMOD_NONE) {
      fprintf(stderr, "%s: move right\n", __func__);
      do_move_right_rpc(ui, C);
      return 2;
    }
    if (sym == SDLK_UP && mod == KMOD_NONE)  {  
      fprintf(stderr, "%s: move up\n", __func__);
      do_move_up_rpc(ui, C);
      return 2;
    }
    if (sym == SDLK_DOWN && mod == KMOD_NONE)  {
      fprintf(stderr, "%s: move down\n", __func__);
      do_move_down_rpc(ui, C);
      return 2;
    }
    if (sym == SDLK_f && mod == KMOD_NONE)  {  
      fprintf(stderr, "%s: pick up flag\n", __func__);
      do_pickup_flag_rpc(ui, C);
      return 2;
      //fprintf(stderr, "%s: dummy pickup red flag\n", __func__);
      //return 2;//ui_pickup_red(ui);
    }
    if (sym == SDLK_h && mod == KMOD_NONE)  {
      fprintf(stderr, "%s: pick up hammer\n", __func__);
      do_pickup_hammer_rpc(ui, C);
      return 2;
    }
    if (sym == SDLK_j && mod == KMOD_NONE)  {   
      fprintf(stderr, "%s: dummy jail\n", __func__);
      return 2;//ui_jail(ui);
    }
    if (sym == SDLK_n && mod == KMOD_NONE)  {   
      fprintf(stderr, "%s: dummy normal state\n", __func__);
      return 2;//ui_normal(ui);
    }
    /*if (sym == SDLK_t && mod == KMOD_NONE)  {   
      fprintf(stderr, "%s: dummy toggle team\n", __func__);
      return ui_dummy_toggle_team(ui);
    }
    if (sym == SDLK_i && mod == KMOD_NONE)  {   
      fprintf(stderr, "%s: dummy inc player id \n", __func__);
      return ui_dummy_inc_id(ui);
    }*/
    if (sym == SDLK_q) return -1;
    if (sym == SDLK_z && mod == KMOD_NONE) {
      if (ui_zoom(ui, 1)==2) {
        Player *p = (Player *)C->data;
        ui_center_cam(ui, &p->pos);
        return 2;
      }
    }
    if (sym == SDLK_z && mod & KMOD_SHIFT) {
      if (ui_zoom(ui, -1)==2) {
        Player *p = (Player *)C->data;
        ui_center_cam(ui, &p->pos);
        return 2;
      }
    }
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

void init_client_player(Client *C) {
  C->data = (Player *)malloc(sizeof(Player));
  if (C->data==NULL) return;

  bzero(C->data, sizeof(Player));
}

//old client helper functions:
int
clientInit(Client *C)
{
  bzero(C, sizeof(Client));

  // initialize the client protocol subsystem
  if (proto_client_init(&(C->ph))<0) {
    fprintf(stderr, "client: main: ERROR initializing proto system\n");
    return -1;
  }
  init_client_player(C);
  return 1;
}

int
startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h)
{
  int player_id = -1;
  int team_num = -1;
  Tuple pos_tuple = {-1, -1};

  if (globals.host[0]!=0 && globals.port!=0) {
    if(proto_client_connect(C->ph, host, port, &player_id, &team_num, &pos_tuple)<0) {
      fprintf(stderr, "failed to connect\n");
      return -1;
    }

    proto_session_set_data(proto_client_event_session(C->ph), C);
    if (h != NULL) {// THIS IS KEY - this is where we set event handlers
      proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_UPDATE, update_event_handler);
      proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_HELLO, hello_event_handler);
      proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_GOODBYE, goodbye_event_handler);
    }

    // initialize the player before we return
    Player *p = (Player *)(C->data);
    pthread_mutex_lock(&p->lock);
      p->id = player_id;
      p->pos.x = pos_tuple.x;
      p->pos.y = pos_tuple.y;
      p->team = team_num;
      p->team_color = (Color)team_num;
      ui_uip_init(ui, &(p->uip), p->id, p->team); // init ui component
    pthread_mutex_unlock(&p->lock);
    return 1;
  }
  return 0;
}

int
startDisconnection(Client *C)
{
  Proto_Session* se = proto_client_event_session(C->ph);
  close(se->fd);
  int rc = proto_client_goodbye(C->ph);
  // close connection to rpc and event channel
  Proto_Session* sr = proto_client_rpc_session(C->ph);
  close(sr->fd);
  globals.connected = 0;
  return 0;
}

// old client commands:
int
doConnect(Client *C)
{
  globals.port=0;
  globals.host[0]=0;
  int i, len = strlen(globals.in.data);

  //VPRINTF("BEGIN: %s\n", globals.in.data);

  if (globals.connected==1) {
     fprintf(stderr, "Already connected to server"); //do nothing
     //fprintf(stderr, "\n"); //do nothing
     return 1;
  } else {
    for (i=0; i<len; i++) if (globals.in.data[i]==':') globals.in.data[i]=' ';
    sscanf(globals.in.data, "%*s %" XSTR(STRLEN) "s %d", globals.host,
	   &globals.port);
    
    if (strlen(globals.host)==0 || globals.port==0) {
      fprintf(stderr, "Not able to connect to <%s:%d>\n", globals.host, globals.port);
      return 1;
    } else {
      // ok startup our connection to the server
      if (startConnection(C, globals.host, globals.port, update_event_handler)<0) {
        fprintf(stderr, "Not able to connect to <%s:%d>\n", globals.host, globals.port);
        return 1;
      }
    }
  }
  globals.connected = 1;
  fprintf(stdout, "Connected to <%s:%d>\n", globals.host, globals.port);
  //VPRINTF("END: %s %d %d\n", globals.server, globals.port, globals.serverFD);
  
  return 1;
}
/*
int
doDisconnect(Client *C)
{
  if (globals.connected == 0)
    return 1; // do nothing
  if (startDisconnection(C)<0)
  {// always returns 0 so this never hits
    fprintf(stderr, "Not able to disconnect from <%s:%d>\n", globals.host, globals.port);
    return 1;
  }
  fprintf(stdout, "Disconnected\n");
  return 1;
}
*/
/*
int
doEnter(Client *C)
{
  //printf("pressed enter\n");
  return 1;
}
*/
/*extern int
proto_client_event_update_handler(Proto_Session *s)
{
  fprintf(stderr,
          "proto_client_event_update_handler: invoked for session:\n");
  proto_session_dump(s);
  Proto_Msg_Types mt;
        
  mt = proto_session_hdr_unmarshall_type(s);
  char board[9];
      
  if (mt == PROTO_MT_EVENT_BASE_UPDATE){
    //update client code should go here -WA 
    proto_session_body_unmarshall_bytes(s, 0, sizeof(globals.map), (char *)&globals.map);
  }

  return 1;
}
*/
/*
int
doMapDump(Client *C)
{
  int rc = 0;
  //printf("pressed dump\n");
  if (globals.connected!=1) {
    fprintf(stderr, "You are not connected\n"); //do nothing
    return 1;
  }
  rc = proto_client_map_dump(C->ph);
  if (rc < 0) {
    fprintf(stderr, "Something went wrong with dump.\n");
    return 1; // temporarily dont quit
  } else {
    fprintf(stdout, "Dumped on server.\n");
  }
  return 1;
}

int
doMapInfoTeam(Client *C, char c)
{
  //printf("pressed %c \n", c);
  int team_num = 0;
  int rc = 0;
  Pos tuple = {0, 0};  

  if (globals.connected!=1) {
     fprintf(stderr, "You are not connected\n"); //do nothing
     return 1;
  } else {
    sscanf(globals.in.data, "%*s %d", &team_num);
  }
  if (team_num == 1) rc = proto_client_map_info_team_1(C->ph, &tuple);
  else if (team_num == 2) rc = proto_client_map_info_team_2(C->ph, &tuple);
  else {
    fprintf(stderr, "Invalid team number\n");
    return 1;
  }
  if (rc < 0) {
    fprintf(stderr, "Something went wrong with %c.\n", c);
    return 1; // temporarily dont quit
  } else {
    if (c == 'h') fprintf(stdout, "numhome=%d\n", tuple.x);
    if (c == 'j') fprintf(stdout, "numjail=%d\n", tuple.y);
  }
  return rc;
}

int
doMapInfo(Client *C, char c)
{
  //printf("pressed %c \n", c);
  int rc = 0;
  Pos tuple = {0, 0};

  if (globals.connected!=1) {
     fprintf(stderr, "You are not connected\n"); //do nothing
     return 1;
  }
  rc = proto_client_map_info(C->ph, &tuple);
  if(rc < 0) {
    fprintf(stderr, "Something went wrong with %c.\n", c);
    return 1; // temporarily dont quit
  } else {
    if (c == 'w') fprintf(stdout, "numwall=%d\n", tuple.x);
    if (c == 'f') fprintf(stdout, "numfloor=%d\n", tuple.y);
  }
  return rc;
}

int
doMapDim(Client *C)
{
  //printf("pressed dim \n");
  int rc = 0;
  Pos dim = {0, 0};
  //dim->x = 0; dim->y = 0;
  if (globals.connected!=1) {
     fprintf(stderr, "You are not connected\n"); //do nothing
     return 1;
  }
  rc = proto_client_map_dim(C->ph, &dim);
  if (rc < 0) {
    fprintf(stderr, "Something went wrong with dim.\n");
    return 1; // temporarily dont quit
  } else {
    fprintf(stdout, "Maze Dimensions: %d x %d (width x height)\n", dim.x, dim.y);
  }
  
  return rc;
}

int
doMapCinfo(Client *C)
{
  //printf("pressed cinfo \n");
  int rc = 0;
  int x = -1;
  int y = -1;
  Cell_Type cell_type;
  int team = 0;
  int occupied = -1;
  if (globals.connected!=1) {
     fprintf(stderr, "You are not connected\n"); //do nothing
     return 1;
  }
  int i, len = strlen(globals.in.data);
  for (i=0; i<len; i++) if (globals.in.data[i]==',') globals.in.data[i]=' ';
  sscanf(globals.in.data, "%*s %d %d", &x, &y);
  if (x < 0 || y < 0 || x > MAPWIDTH-1 || y > MAPHEIGHT-1) {
    fprintf(stderr, "Invalid coordinates.\n");
    return 1;
  }
  Pos pos = {x, y};
  
  rc = proto_client_map_cinfo(C->ph, &pos, &cell_type, &team, &occupied);
  if (rc < 0) {
    fprintf(stderr, "Something went wrong with cinfo, try again later.\n");
    return 1; // temporarily dont quit
  } else {
    fprintf(stdout, "Cell Info for <%d,%d>: Cell Type: %d, Team: %d, Occupied: %d\n", x, y, cell_type, team, occupied);
  }
  
  return rc;
}
*/
int
doQuit(Client *C)
{
  //printf("quit pressed\n");
  if (globals.connected == 1) {
    // disconnect first
    //if (startDisconnection(C)<0) printf("Not able to disconnect. Quitting.\n");
    //else fprintf(stdout, "Disconnected.\n");
  }
  return -1;
}

