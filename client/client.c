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
#include "../ui/uistandalone.h"

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
  switch (cmd) {
  case 'q':
    printf("q ->quitting...\n");
    rc=-1;
    break;
  case 'a':
    printf("a ->do rpc: up\n");
    //ui_up(ui);
    rc=2;
    break;
  case 'z':
    printf("z ->do rpc: down\n");
    //ui_down(ui);
    rc=2;
    break;
  case ',':
    printf("u ->do rpc: left\n");
    //ui_left(ui);
    rc=2;
    break;
  case '.':
    printf(". ->do rpc: right\n");
    //ui_right(ui);
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
  Client *C = proto_session_get_data(s);
  proto_session_body_unmarshall_bytes(s, 0, getMapSize(), getMapBufPointer());
  convertMap();
  
  //ui_paintmap(ui, &globals.map); TODO:FIX this is okay for now, but if we do actual event updates, we need to paint here, because the ui main loop will already be running. maybe we can check to see if the main has started, print, if not, dont print

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

  // RUN AUTO-CONNECT FIRST
  if (startConnection(&c, globals.host, globals.port, update_event_handler)<0) return -1;
  else {
    globals.connected = 1;
    fprintf(stdout, "Connected to <%s:%d>\n", globals.host, globals.port); 
  }
  // END CONNECT

  pthread_t tid;
  pthread_create(&tid, NULL, shell, &c);

  // WITH OSX ITS IS EASIEST TO KEEP UI ON MAIN THREAD
  // SO JUMP THROW HOOPS :-(
  
  proto_debug_on();
  ui_client_main_loop(ui, (void *)&globals.map); //TODO:FIX if the update_event_handler for hello is not hit before this, then the map will not be initialized and the main loop will just print all floor (JAIL) cells until the handler is hit.
  return 0;
}

extern sval
ui_keypress(UI *ui, SDL_KeyboardEvent *e)
{
  SDLKey sym = e->keysym.sym;
  SDLMod mod = e->keysym.mod;

  if (e->type == SDL_KEYDOWN) {
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
    /*if (sym == SDLK_t && mod == KMOD_NONE)  {   
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
    p->id = player_id;
    p->pos.x = pos_tuple.x;
    p->pos.y = pos_tuple.y;
    p->team = team_num;
    p->team_color = (Color)team_num;
    ui_uip_init(ui, &(p->uip), p->id, p->team); // init ui component

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

int
doEnter(Client *C)
{
  //printf("pressed enter\n");
  return 1;
}
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
    if (startDisconnection(C)<0) printf("Not able to disconnect. Quitting.\n");
    else fprintf(stdout, "Disconnected.\n");
  }
  return -1;
}

