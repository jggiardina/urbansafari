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
  char Board_Init[9];
  int data;
  char player_type;
  Proto_Client_Handle ph;
} Client;

static int
clientInit(Client *C)
{
  bzero(C, sizeof(Client));
  
  // initialize player_type to '?'
  C->player_type = '?';

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
    char player_type = proto_client_connect(C->ph, host, port, C->Board_Init);
    //memcpy(C->Board_Init, rcAndBoard+1, 9);
    if (player_type == 'F') {
      //fprintf(stderr, "failed to connect\n");
      return player_type;
    }
    proto_session_set_data(proto_client_event_session(C->ph), C);
#if 0
    if (h != NULL) {
      proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_UPDATE, 
				     h);
    }
#endif
    return player_type;
  }
  return 'F';
}

int
startDisconnection(Client *C, char *host, PortType port)
{
  if (globals.host[0]!=0 && globals.port!=0) {
    if(proto_client_disconnect(C->ph, host, port)<0)
      return -1;
  }
  globals.connected = 0;
  C->player_type = '?';
  return 0;
}


int
prompt(int menu, char player_type) 
{
  //static char MenuString[] = "\n%s> ";
  int len;
  //int c=0;

  if (menu) printf("\n%c>", player_type);
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
game_process_mark_reply(Client *C, int rc)
{
  // NOT sure why this is even here. -JG
  //Proto_Session *s;

  //s = proto_client_rpc_session(C->ph);

  //fprintf(stderr, "%s: do something %p\n", __func__, s);

  // I will use this function to process the reply from MARK
  // rc = 0: "Game hasn't started"
  // rc = 1: "Valid Move"
  // rc = 2: "Invalid Move"
  // rc = 3: "Not your turn"
  switch (rc)
  {
    case 0:
      printf("Game hasn't started yet\n");
      break;
    case 2:
      printf("Not a valid move!\n");
      break;
    case 3:
      printf("Not your turn yet!\n");
      break;
    default: // rc = 1
      break;
  }

  return rc;
}


int 
doMarkRPCCmd(Client *C, int c) 
{
  int rc=-1;

  rc = proto_client_mark(C->ph, c, C->player_type); //TODO: change this to mark
  
  //printf("mark: rc=%d\n", rc);
  if (rc > 0) game_process_mark_reply(C, rc);
  else printf("Game hasn't started yet\n");  
  return rc;
  // NULL MT OVERRIDE ;-)
  // printf("%s: rc=0x%x\n", __func__, rc);
  // if (rc == 0xdeadbeef) rc=1;
  // return rc;
}

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
      //printf("enter (h|m<c>|g): ");
      //scanf("%c", &c);
      rc=doMarkRPCCmd(C,c);

      //printf("doRPC: rc=%d\n", rc);
    }
    return rc==1 ? 0 : 1;
  } else {
    // not connected so do nothing
    printf("You are not connected");
    return 1;
  }
}

int
doConnect(Client *C)
{
  globals.port=0;
  globals.host[0]=0;
  int i, len = strlen(globals.in.data);

  //VPRINTF("BEGIN: %s\n", globals.in.data);

  if (globals.connected==1) {
     //fprintf(stderr, "Already connected to server"); //do nothing
     fprintf(stderr, "\n"); //do nothing
  } else {
    for (i=0; i<len; i++) if (globals.in.data[i]==':') globals.in.data[i]=' ';
    sscanf(globals.in.data, "%*s %" XSTR(STRLEN) "s %d", globals.host,
	   &globals.port);
    
    if (strlen(globals.host)==0 || globals.port==0) {
      fprintf(stderr, "Not able to connect to <%s:%d>\n", globals.host, globals.port);
      return 1;
    } else {
      //VPRINTF("connecting to: server=%s port=%d...", 
	//      globals.server, globals.port);
      /*if (net_setup_connection(&globals.serverFD, globals.server, globals.port)<0) {
	fprintf(stderr, " failed NOT connected server=%s port=%d\n", 
		globals.server, globals.port);
      } else {
	globals.connected=1;
	VPRINTF("connected serverFD=%d\n", globals.serverFD);
      }*/
      // ok startup our connection to the server
      char player_type = startConnection(C, globals.host, globals.port, update_event_handler);
      if (player_type == 'F') {
        fprintf(stderr, "Not able to connect to <%s:%d>\n", globals.host, globals.port);
        return 1;
      } else {
        globals.connected = 1;
	C->player_type = player_type;
        printf("Connected to <%s:%d>: You are %c's", globals.host, globals.port, C->player_type);
        printGameBoard(C->Board_Init);
      }
    }
  }

  //VPRINTF("END: %s %d %d\n", globals.server, globals.port, globals.serverFD);
  return 1;
}
//
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

int
doEnter(Client *C)
{
  //printf("pressed enter\n");
  if (globals.connected == 1)
    proto_client_print_board(C->ph);
  return 1;
}

int
doWhere(Client *C)
{
  // TEST
  proto_client_print_board(C->ph);
  doMarkRPCCmd(C, 1);
  //printf("pressed enter\n");
  // TEST
  if (globals.connected == 1)
    printf("<%s:%d>\n", globals.host, globals.port);
  else
    printf("not connected\n");
  return 1;
}

int
doQuit(Client *C)
{
  //printf("quit pressed\n");
  if (globals.connected == 1) {
    // disconnect first
    startDisconnection(C, globals.host, globals.port);
    printf("Game Over: You Quit\n");
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
  else if (strncmp(globals.in.data, "disconnect", 
		   sizeof("disconnect")-1)==0) rc = doDisconnect(C);
  else if (strncmp(globals.in.data, "quit", 
		   sizeof("quit")-1)==0) rc = doQuit(C);
  else if (strncmp(globals.in.data, "where",
		   sizeof("where")-1)==0) rc = doWhere(C);
  else rc = doMarkRPC(C);

  return rc;
  /*
  switch (cmd) {
  case 'd':
    proto_debug_on();
    break;
  case 'D':
    proto_debug_off();
    break;
  case 'r':
    rc = doRPC(C);
    break;
  case 'q':
    rc=-1;
    break;
  case '\n':
    rc=1;
    break;
  default:
    printf("Unkown Command\n");
  }
  return rc;*/
}

void *
shell(void *arg)
{
  Client *C = arg;
  //char c;
  int rc;
  int menu=1;

  while (1) {
    if ((prompt(menu, C->player_type))!=0) rc=docmd(C); else rc = -1;
    if (rc<0) break;
    if (rc==1) menu=1; else menu=0;
  }

  //fprintf(stderr, "terminating\n");
  fflush(stdout);
  return NULL;
}

void 
usage(char *pgm)
{
  fprintf(stderr, "USAGE: %s <port|<<host port> [shell] [gui]>>\n"
           "  port     : rpc port of a game server if this is only argument\n"
           "             specified then host will default to localhost and\n"
	   "             only the graphical user interface will be started\n"
           "  host port: if both host and port are specifed then the game\n"
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

int 
main(int argc, char **argv)
{
  Client c;
  //initGlobals(argc, argv); // Don't init globals, just zero them out
  bzero(&globals, sizeof(globals));
  
  if (clientInit(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }    

  /* MOVE TO doConnect -JG
  // ok startup our connection to the server
  if (startConnection(&c, globals.host, globals.port, update_event_handler)<0) {
    fprintf(stderr, "ERROR: startConnection failed\n");
    return -1;
  }
  */

  shell(&c);

  return 0;
}

