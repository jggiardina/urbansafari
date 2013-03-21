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
#include <sys/types.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_client.h"
#include "misc.h"
//#include "maze.h"

typedef struct {
  Proto_Session rpc_session;
  Proto_Session event_session;
  pthread_t EventHandlerTid;
  Proto_MT_Handler session_lost_handler;
  Proto_MT_Handler base_event_handlers[PROTO_MT_EVENT_BASE_RESERVED_LAST 
				       - PROTO_MT_EVENT_BASE_RESERVED_FIRST
				       - 1];
} Proto_Client;

extern Proto_Session *
proto_client_rpc_session(Proto_Client_Handle ch)
{
  Proto_Client *c = ch;
  return &(c->rpc_session);
}

extern Proto_Session *
proto_client_event_session(Proto_Client_Handle ch)
{
  Proto_Client *c = ch;
  return &(c->event_session);
}

extern int
proto_client_set_session_lost_handler(Proto_Client_Handle ch, Proto_MT_Handler h)
{
  Proto_Client *c = ch;
  c->session_lost_handler = h;
}

extern int
proto_client_set_event_handler(Proto_Client_Handle ch, Proto_Msg_Types mt,
			       Proto_MT_Handler h)
{
  int i;
  Proto_Client *c = ch;

  if (mt>PROTO_MT_EVENT_BASE_RESERVED_FIRST && 
      mt<PROTO_MT_EVENT_BASE_RESERVED_LAST) {
    i=mt - PROTO_MT_EVENT_BASE_RESERVED_FIRST - 1;
    //ADD CODE: Added this line to set the proper handler based on the message type. - JG
    c->base_event_handlers[i] = h;
    return 1;
  } else {
    return -1;
  }
}

static int 
proto_client_session_lost_default_hdlr(Proto_Session *s)
{
  fprintf(stderr, "Session lost...:\n");
  proto_session_dump(s);
  return -1;
}

static int 
proto_client_event_null_handler(Proto_Session *s)
{
  fprintf(stderr, 
	  "proto_client_event_null_handler: invoked for session:\n");
  proto_session_dump(s);
  
  return 1;
}
/* ALL EVENT HANDLERS NOW IN client/client.c -JG
static int
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
    proto_session_body_unmarshall_bytes(s, 0, sizeof(board), (char *)&board);
    printGameBoardFromEvent(&board);
    printMarker();  
    // print marker here too.
    

    proto_session_reset_send(s);//now to send back ACK message
    Proto_Msg_Hdr h;
    bzero(&h, sizeof(h));
    h.type = mt;
    proto_session_hdr_marshall(s, &h);
    proto_session_send_msg(s, 1);
  }

  return 1;
}

static int
proto_client_event_finish_handler(Proto_Session *s)
{
  fprintf(stderr,
          "proto_client_event_finish_handler: invoked for session:\n");
  proto_session_dump(s);
  Proto_Msg_Types mt;
  Proto_Msg_Hdr hdr;
  char player;
  char board[9];
  
  mt = proto_session_hdr_unmarshall_type(s);
  if (mt == PROTO_MT_EVENT_BASE_WIN){
    //update client code should go here -WA
    proto_session_hdr_unmarshall(s, &hdr);

    player = (char)hdr.pstate.v0.raw;    
    if (PLAYER_INFO_GLOBALS.player_type == player)
      fprintf(stderr, "\nGame Over: You win");
    else
      fprintf(stderr, "\nGame Over: You lose");
    
    proto_session_reset_send(s);//now to send back ACK message
    Proto_Msg_Hdr h;
    bzero(&h, sizeof(h));
    h.type = mt;
    proto_session_hdr_marshall(s, &h);
    proto_session_send_msg(s, 1);
  }else{ //DRAW
     fprintf(stderr, "\nGame Over: Draw");
     proto_session_reset_send(s);//now to send back ACK message
    Proto_Msg_Hdr h;
    bzero(&h, sizeof(h));
    h.type = mt;
    proto_session_hdr_marshall(s, &h);
    proto_session_send_msg(s, 1);
  }
  proto_session_body_unmarshall_bytes(s, 0, sizeof(board), (char *)&board);
  printGameBoardFromEvent(&board);
  printMarker();
  return 1;
}
*/

/*
static int
proto_client_event_disconnect_handler(Proto_Session *s)
{
  fprintf(stderr,
          "proto_client_event_disconnect_handler: invoked for session:\n");
  proto_session_dump(s);
  Proto_Msg_Types mt;

  int ret = 1;
  int player_quit=-1;

  mt = proto_session_hdr_unmarshall_type(s);

  if(mt == PROTO_MT_EVENT_BASE_DISCONNECT){
   proto_session_body_unmarshall_int(s, 0, &player_quit);
   // Below if statement...kludgy, because the player could reconnect as not the 0 or 1 spot and then this no longer works, but because we don't care about what happens after the game is over we leave this for now. -JG
   if((player_quit == 1 && PLAYER_INFO_GLOBALS.player_type == 'X') || (player_quit == 0 && PLAYER_INFO_GLOBALS.player_type == 'O')){ 
     fprintf(stderr, "\nGame Over: Other Side Quit");
     printMarker();
   }
  }

  return ret;
}
*/
static void *
proto_client_event_dispatcher(void * arg)
{
  Proto_Client *c;
  Proto_Session *s;
  Proto_Msg_Types mt;
  Proto_MT_Handler hdlr;
  int i;

  pthread_detach(pthread_self());

  c = arg; //ADD CODE: arg passed here on pthread_create is Proto_Client *. -JG
  s = proto_client_event_session(c); //ADD CODE: set the Proto_Session, event_session is only Proto_Session, so we need it's address here. -JG

  for (;;) {
    if (proto_session_rcv_msg(s)==1) {
      mt = proto_session_hdr_unmarshall_type(s);
      if (mt > PROTO_MT_EVENT_BASE_RESERVED_FIRST && 
	  mt < PROTO_MT_EVENT_BASE_RESERVED_LAST) {
	//ADD CODE: Probably want to set hdlr with c->base_event_handlers[i], where i=mt - PROTO_MT_EVENT_BASE_RESERVED_FIRST - 1; -JG
	i=mt - PROTO_MT_EVENT_BASE_RESERVED_FIRST - 1;
        hdlr = c->base_event_handlers[i];
        if (hdlr(s)<0) goto leave;
      }
    } else {
      //ADD CODE: maybe set the sessions_lost handler but not sure. -JG
      proto_client_set_session_lost_handler(c,
                                proto_client_session_lost_default_hdlr);
      goto leave;
    }
  }
 leave:
  close(s->fd);
  return NULL;
}

extern int
proto_client_init(Proto_Client_Handle *ch)
{
  Proto_Msg_Types mt;
  Proto_Client *c;

  c = (Proto_Client *)malloc(sizeof(Proto_Client));
  if (c==NULL) return -1;
  bzero(c, sizeof(Proto_Client));

  proto_client_set_session_lost_handler(c, 
			      	proto_client_session_lost_default_hdlr);

  for (mt=PROTO_MT_EVENT_BASE_RESERVED_FIRST+1;
       mt<PROTO_MT_EVENT_BASE_RESERVED_LAST; mt++)
    //ADD CODE: probably looping through the base_event_handlers and initializing them to null here. -JG
    proto_client_set_event_handler(c, mt, proto_client_event_null_handler);
  *ch = c;
  return 1;
}

int
proto_client_connect(Proto_Client_Handle ch, char *host, PortType port)
{
  Proto_Client *c = (Proto_Client *)ch;

  if (net_setup_connection(&(c->rpc_session.fd), host, port)<0) 
    return -1;

  if (net_setup_connection(&(c->event_session.fd), host, port+1)<0) 
    return -2; 

  if (pthread_create(&(c->EventHandlerTid), NULL, 
		     &proto_client_event_dispatcher, c) !=0) {
    fprintf(stderr, 
	    "proto_client_init: create EventHandler thread failed\n");
    perror("proto_client_init:");
    return -3;
  }

  int rc = proto_client_hello(ch);
  return rc;
}

static void
marshall_mtonly(Proto_Session *s, Proto_Msg_Types mt) {
  Proto_Msg_Hdr h;
  
  bzero(&h, sizeof(h));
  h.type = mt;
  proto_session_hdr_marshall(s, &h);
};

// all rpc's are assume to only reply only with a return code in the body
// eg.  like the null_mes
static int 
do_generic_dummy_rpc(Proto_Client_Handle ch, Proto_Msg_Types mt)
{
  int rc;
  Proto_Session *s;
  Proto_Client *c = ch;

  s = proto_client_rpc_session(c); //ADD CODE: set the Proto_Session, rpc_session is only Proto_Session, so we need it's address here. -JG
  
  // marshall
  marshall_mtonly(s, mt);
  rc = proto_session_rpc(s);//perform our rpc call
  if (rc==1) {
    proto_session_body_unmarshall_int(s, 0, (int *)&rc); 
  } else {
    //ADD CODE send_msg communication failed so assign the session lost handler and close the session. -JG
    c->session_lost_handler(s);
    close(s->fd);
  }
  
  return rc;
}

static int
do_unmarshall_two_ints_from_body_rpc(Proto_Client_Handle ch, Proto_Msg_Types mt, Pos *dim)
{
  int rc;
  Proto_Session *s;
  Proto_Client *c = ch;

  s = proto_client_rpc_session(c); //ADD CODE: set the Proto_Session, rpc_session is only Proto_Session, so we need it's address here. -JG

  // marshall
  marshall_mtonly(s, mt);
  rc = proto_session_rpc(s);//perform our rpc call
  if (rc==1) {
    proto_session_body_unmarshall_int(s, 0, &(dim->x));
    proto_session_body_unmarshall_int(s, sizeof(int), &(dim->y));
  } else {
    //ADD CODE send_msg communication failed so assign the session lost handler and close the session. -JG
    c->session_lost_handler(s);
    close(s->fd);
  }

  return rc;
}

static int
do_map_cinfo_rpc(Proto_Client_Handle ch, Proto_Msg_Types mt, Pos *pos, Cell_Type *cell_type, int *team, int *occupied)
{
  int rc;
  Proto_Session *s;
  Proto_Client *c = ch;

  s = proto_client_rpc_session(c); //ADD CODE: set the Proto_Session, rpc_session is only Proto_Session, so we need it's address here. -JG

  // marshall
  marshall_mtonly(s, mt);
  proto_session_body_marshall_int(s, pos->x);
  proto_session_body_marshall_int(s, pos->y);
  
  rc = proto_session_rpc(s);//perform our rpc call
  if (rc==1) {
    proto_session_body_unmarshall_int(s, 0, (int*)cell_type);
    if (*cell_type==-1)
      return -1;//failed, so return -1
    proto_session_body_unmarshall_int(s, sizeof(int), team);
    proto_session_body_unmarshall_int(s, 2*sizeof(int), occupied);
  } else {
    //ADD CODE send_msg communication failed so assign the session lost handler and close the session. -JG
    c->session_lost_handler(s);
    close(s->fd);
  }

  return rc;
}

extern int 
proto_client_hello(Proto_Client_Handle ch)
{
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_HELLO);  
}

extern int
proto_client_map_info_team_1(Proto_Client_Handle ch, Pos *tuple)
{
  return do_unmarshall_two_ints_from_body_rpc(ch,PROTO_MT_REQ_BASE_MAP_INFO_1, tuple);
}

extern int
proto_client_map_info_team_2(Proto_Client_Handle ch, Pos *tuple)
{
  return do_unmarshall_two_ints_from_body_rpc(ch,PROTO_MT_REQ_BASE_MAP_INFO_2, tuple);
}

extern int
proto_client_map_info(Proto_Client_Handle ch, Pos *tuple)
{
  return do_unmarshall_two_ints_from_body_rpc(ch,PROTO_MT_REQ_BASE_MAP_INFO, tuple);
}

extern int
proto_client_map_dim(Proto_Client_Handle ch, Pos *dim)
{
  return do_unmarshall_two_ints_from_body_rpc(ch,PROTO_MT_REQ_BASE_MAP_DIM, dim);
}

extern int
proto_client_map_cinfo(Proto_Client_Handle ch, Pos *pos, Cell_Type *cell_type, int *team, int *occupied)
{
  return do_map_cinfo_rpc(ch,PROTO_MT_REQ_BASE_MAP_CINFO, pos, cell_type, team, occupied);
}

extern int
proto_client_map_dump(Proto_Client_Handle ch)
{
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_MAP_DUMP);
}

/*
extern int proto_client_disconnect(Proto_Client_Handle ch, char *host, PortType port){
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_DISCONNECT);  
}
*/
/*
extern int 
proto_client_move(Proto_Client_Handle ch, char data)
{
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_MOVE);  
}
*/
extern int 
proto_client_goodbye(Proto_Client_Handle ch/*, char *host, PortType port*/)
{
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_GOODBYE);  
}
