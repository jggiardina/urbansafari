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
#include "../lib/types.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"
//#include "../lib/maze.h"

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
} globals;

typedef struct ClientState  {
  int data;
  Proto_Client_Handle ph;
} Client;

static int
clientInit(Client *C)
{
  bzero(C, sizeof(Client));
  
  // initialize the client protocol subsystem
  if (proto_client_init(&(C->ph))<0) {
    fprintf(stderr, "client: main: ERROR initializing proto system\n");
    return -1;
  }
  return 1;
}


static int
update_event_handler(Proto_Session *s)
{
  Client *C = proto_session_get_data(s);

  fprintf(stderr, "%s: called", __func__);
  return 1;
}


char 
startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h)
{
  if (globals.host[0]!=0 && globals.port!=0) {
    if(proto_client_connect(C->ph, host, port)!=0) {
      fprintf(stderr, "failed to connect\n");
      return -1;
    }
    proto_session_set_data(proto_client_event_session(C->ph), C);
    if (h != NULL) {// THIS IS KEY - this is where we set event handlers
      proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_UPDATE, 
				     h);
    }
    return 1;
  }
  return 0;
}

int
startDisconnection(Client *C, char *host, PortType port)
{
  if (globals.host[0]!=0 && globals.port!=0) {
    if(proto_client_goodbye(C->ph/*, host, port*/)<0)
      return -1;
  }
  globals.connected = 0;
  //C->player_type = '?';
  return 0;
}


int
prompt(int menu) 
{
  static char MenuString[] = "\nclient> ";
  int len;

  if (menu) printf("%s", MenuString);
  fflush(stdout);
  len = getInput();
  return (len) ? 1 : -1;
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

// FIXME:  this is ugly maybe the speration of the proto_client code and
//         the game code is dumb
int
game_process_reply(Client *C)
{
  Proto_Session *s;

  s = proto_client_rpc_session(C->ph);

  fprintf(stderr, "%s: do something %p\n", __func__, s);

  return 1;
}

/*int 
doMarkRPCCmd(Client *C, int c) 
{
  int rc=-1;

  rc = proto_client_mark(C->ph, c); 
  
  if (rc > 0) game_process_mark_reply(C, rc);
  else printf("Game hasn't started yet\n");  
  return rc;
}*/
/*
int
doMarkRPC(Client *C)
{
  if (globals.connected == 1) {
    int rc;
    int c = atoi(globals.in.data);
    if (c < 1 || c > 9 )
    {
      printf("Not a valid move!\n");
      return 1;
    }
    else
    {
      rc=doMarkRPCCmd(C,c);
    }
    return rc==1 ? 0 : 1;
  } else {
    // not connected so do nothing
    printf("You are not connected");
    return 1;
  }
}
*/
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
  fprintf(stderr, "Connected to <%s:%d>\n", globals.host, globals.port);
  //VPRINTF("END: %s %d %d\n", globals.server, globals.port, globals.serverFD);
  return 1;
}
/*
int
doDisconnect(Client *C)
{
  if (globals.connected == 0)
    return 1; // do nothing
  if (startDisconnection(C, globals.host, globals.port)<0)
  {
    fprintf(stderr, "Not able to disconnect from <%s:%d>\n", globals.host, globals.port);
        return 1;
  }
  printf("Game Over: You Quit\n");
  return 1;
}
*/
int
doEnter(Client *C)
{
  printf("pressed enter\n");
  return 1;
}

int
doMapDump(Client *C)
{
  int rc = 0;
  printf("pressed dump\n");
  if (globals.connected!=1) {
    fprintf(stderr, "You are not connected\n"); //do nothing
    return 1;
  }
  rc = proto_client_map_dump(C->ph);
  if (rc < 0) {
    fprintf(stderr, "Something went wrong with dump.\n");
    return 1; // temporarily dont quit
  } else {
    fprintf(stderr, "Dumped on server.\n");
  }
  return 1;
}

int
doMapInfoTeam(Client *C, char c)
{
  printf("pressed %c \n", c);
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
    if (c == 'h') fprintf(stderr, "numhome=%d\n", tuple.x);
    if (c == 'j') fprintf(stderr, "numjail=%d\n", tuple.y);
  }
  return rc;
}

int
doMapInfo(Client *C, char c)
{
  printf("pressed %c \n", c);
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
    if (c == 'w') fprintf(stderr, "numwall=%d\n", tuple.x);
    if (c == 'f') fprintf(stderr, "numfloor=%d\n", tuple.y);
  }
  return rc;
}

int
doMapDim(Client *C)
{
  printf("pressed dim \n");
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
    fprintf(stderr, "Maze Dimensions: %d x %d (width x height)\n", dim.x, dim.y);
  }
  
  return rc;
}

int
doMapCinfo(Client *C)
{
  printf("pressed cinfo \n");
  int rc = 0;
  int x,y = 0;
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
  if (x < 0 || y < 0) {
    fprintf(stderr, "Invalid coordinates: <%d,%d>, x and y cannot be negative", x, y);
    return 1;
  }
  Pos pos = {x, y};
  
  rc = proto_client_map_cinfo(C->ph, &pos, &cell_type, &team, &occupied);
  if (rc < 0) {
    fprintf(stderr, "Something went wrong with cinfo, check coords.\n");
    return 1; // temporarily dont quit
  } else {
    fprintf(stderr, "Cell Info for <%d,%d>: Cell Type: %d, Team: %d, Occupied: %d\n", x, y, cell_type, team, occupied);
  }
  
  return rc;
}

int
doQuit(Client *C)
{
  printf("quit pressed\n");
  if (globals.connected == 1) {
    // disconnect first
    //startDisconnection(C, globals.host, globals.port);
    //printf("Game Over: You Quit\n");
  }
  return -1;
}

int 
docmd(Client *C)
{
  int rc = 1;
  
  if (strlen(globals.in.data)==0) rc = doEnter(C);
  else if (strncmp(globals.in.data, "connect", 
		   sizeof("connect")-1)==0) rc = doConnect(C);
  /*else if (strncmp(globals.in.data, "disconnect", 
		   sizeof("disconnect")-1)==0) rc = doDisconnect(C);*/
  else if (strncmp(globals.in.data, "quit", 
		   sizeof("quit")-1)==0) rc = doQuit(C);
  else if (strncmp(globals.in.data, "numhome",
		   sizeof("numhome")-1)==0) rc = doMapInfoTeam(C, 'h');
  else if (strncmp(globals.in.data, "numjail",
		   sizeof("numjail")-1)==0) rc = doMapInfoTeam(C, 'j');
  else if (strncmp(globals.in.data, "numwall",
		   sizeof("numwall")-1)==0) rc = doMapInfo(C, 'w');
  else if (strncmp(globals.in.data, "numfloor",
		   sizeof("numfloor")-1)==0) rc = doMapInfo(C, 'f');
  else if (strncmp(globals.in.data, "dump",
		   sizeof("dump")-1)==0) rc = doMapDump(C);
  else if (strncmp(globals.in.data, "dim",
                   sizeof("dim")-1)==0) rc = doMapDim(C);
  else if (strncmp(globals.in.data, "cinfo",
                   sizeof("cinfo")-1)==0) rc = doMapCinfo(C);
  else {
    fprintf(stderr, "Invalid command\n");
    rc = 1;
  }

  return rc;
}

void *
shell(void *arg)
{
  Client *C = arg;
  int rc;
  int menu=1;

  while (1) {
    if ((prompt(menu))!=0) rc=docmd(C); else rc = -1;
    if (rc<0) break;
    if (rc==1) menu=1; else menu=0;
  }

  //fprintf(stderr, "terminating\n");
  fflush(stdout);
  return NULL;
}

int 
main(int argc, char **argv)
{
  Client c;
  bzero(&globals, sizeof(globals));
  
  if (clientInit(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }    

  shell(&c);

  return 0;
}

