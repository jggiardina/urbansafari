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

#include <sys/types.h>
#include <poll.h>
#include "../lib/types.h"
#include "../lib/protocol_server.h"
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
} globals;

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
  "d/D-debug on/off u-update clients q-quit";

int 
docmd(char cmd)
{
  int rc = 1;

  if (strlen(globals.in.data)==0) rc = 1;
  else if (strncmp(globals.in.data, "load",
                   sizeof("load")-1)==0) rc = doLoad();
  else if (strncmp(globals.in.data, "dump",
                   sizeof("dump")-1)==0) rc = doDump();

  return rc;
}
int
doLoad(Map map){
  char filename[256];
  char linebuf[240];
  char mapbuf[20000];
  FILE * myfile;
  int i, n, len = strlen(globals.in.data);
  sscanf(globals.in.data, "%*s %s", filename);
  myfile = fopen(filename, "r");
  if ( myfile == 0 ){
    fprintf( stderr, "Could not open file\n" );
  }
        else{
		n = 0;
         while(fgets(linebuf, sizeof(linebuf), myfile)){
		for (i = 0; i < 200; i++){
			mapbuf[i+n] = linebuf[i+n];
			 
			/*if (i > 99) {
				c.c = GREEN;
			}else{
				c.c = RED;
			}
			switch(linebuf[i]){
				case ' ':
					c.t = FLOOR;
					break;
				case '#':
					c.t = WALL;
					break;
				case 'h':
					c.t = HOME;
					c.c = RED;
					break;
				case 'H':
					c.t = HOME;
					c.c = GREEN;
					break;
				case 'j':
					c.t = JAIL;
					c.c = RED;
					break;
				case 'J':
					c.t = JAIL;
					c.c = GREEN;
					break;
				default:
					fprintf(stderr, "Invalid input character: %s", linebuf[i]);
					return -1;
			}
			c.p.x = i;
			c.p.y = n;
			map.cells[i+n] = c;
		}
		n+= 200;
		bzero(linebuf, sizeof(linebuf));
		*/
	}
}
load_map(mapbuf);
    }

  return 1;

}
int
doDump(){
	return 1;
}
int
prompt(int menu) 
{
  int ret;
  int c=0;

  if (menu) printf("%s:", MenuString);
  fflush(stdout);
  c=getInput();
  return c;
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
  if (proto_server_init()<0) {
    fprintf(stderr, "ERROR: failed to initialize proto_server subsystem\n");
    exit(-1);
  }
  fprintf(stderr, "RPC Port: %d, Event Port: %d\n", proto_server_rpcport(), 
	  proto_server_eventport());

  if (proto_server_start_rpc_loop()<0) {
    fprintf(stderr, "ERROR: failed to start rpc loop\n");
    exit(-1);
  }
    
  shell(NULL);

  return 0;
}
