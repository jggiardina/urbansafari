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
#include "../lib/maze.c"
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
  int isLoaded;
  Map map;
  char mapbuf[MAPHEIGHT*MAPWIDTH];
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
		fprintf(stderr, "%s \n", globals.map.data_ascii);
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
	fprintf(stderr, "All the same.\n");
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
