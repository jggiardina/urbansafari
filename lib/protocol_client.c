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

typedef struct {
  Proto_Session rpc_session;
  Proto_Session event_session;
  pthread_t EventHandlerTid;
  Proto_MT_Handler session_lost_handler;
  Proto_MT_Handler base_event_handlers[PROTO_MT_EVENT_BASE_RESERVED_LAST 
				       - PROTO_MT_EVENT_BASE_RESERVED_FIRST
				       - 1];
} Proto_Client;

struct {
  char player_type;
} PLAYER_INFO_GLOBALS; 

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
  Proto_Msg_Types mt;

  mt = proto_session_hdr_unmarshall_type(s);
  if (mt == PROTO_MT_EVENT_BASE_UPDATE){
	//update client code should go here -WA
  	proto_session_reset_send(s);//now to send back ACK message
	Proto_Msg_Hdr h;
	bzero(&h, sizeof(h));
  	h.type = PROTO_MT_EVENT_BASE_UPDATE;
  	proto_session_hdr_marshall(s, &h);
 	proto_session_send_msg(s, 1);
  }
  return 1;
}

static int
proto_client_event_update_handler(Proto_Session *s)
{
  /*fprintf(stderr,
          "proto_client_event_update_handler: invoked for session:\n");
  proto_session_dump(s);*/
  Proto_Msg_Types mt;

  mt = proto_session_hdr_unmarshall_type(s);
  char board[9];
 
  if (mt == PROTO_MT_EVENT_BASE_UPDATE){
    //update client code should go here -WA
    proto_session_body_unmarshall_bytes(s, 0, sizeof(board), &board);
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
  /*fprintf(stderr,
          "proto_client_event_finish_handler: invoked for session:\n");
  proto_session_dump(s);*/
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
  proto_session_body_unmarshall_bytes(s, 0, sizeof(board), &board);
  printGameBoardFromEvent(&board);
  printMarker();
  return 1;
}

static char
proto_client_rpc_conn_handler(Proto_Session *s)
{
  /*fprintf(stderr,
          "proto_client_rpc_conn_handler: invoked for session:\n");
  proto_session_dump(s);*/
  Proto_Msg_Types mt;
  Proto_Msg_Hdr hdr;

  char tag;
  mt = proto_session_hdr_unmarshall_type(s);
  
  if(mt == PROTO_MT_REP_BASE_CONNECT){
    proto_session_hdr_unmarshall(s, &hdr);
    tag = (char)hdr.pstate.v3.raw;
  }

  return tag;
}

static int
proto_client_rpc_disconnect_handler(Proto_Session *s)
{
  /*fprintf(stderr,
          "proto_client_rpc_disconnect_handler: invoked for session:\n");
  proto_session_dump(s);*/
  Proto_Msg_Types mt;

  int ret;
  mt = proto_session_hdr_unmarshall_type(s);

  if(mt == PROTO_MT_REP_BASE_DISCONNECT){
   proto_session_body_unmarshall_int(s, 0, &ret);
  }

  return ret;
}

static int
proto_client_event_disconnect_handler(Proto_Session *s)
{
  /*fprintf(stderr,
          "proto_client_event_disconnect_handler: invoked for session:\n");
  proto_session_dump(s);*/
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
  if(mt == PROTO_MT_REP_BASE_DISCONNECT){
    proto_client_set_event_handler(c, mt, proto_client_rpc_disconnect_handler);
  }else if(mt == PROTO_MT_REP_BASE_CONNECT){
    proto_client_set_event_handler(c, mt, proto_client_rpc_conn_handler);
  }else if(mt == PROTO_MT_EVENT_BASE_DISCONNECT){
    proto_client_set_event_handler(c, mt, proto_client_event_disconnect_handler);
  }else if(mt == PROTO_MT_EVENT_BASE_UPDATE){
    proto_client_set_event_handler(c, mt, proto_client_event_update_handler);
  }else if(mt == PROTO_MT_EVENT_BASE_WIN || mt == PROTO_MT_EVENT_BASE_DRAW){
    proto_client_set_event_handler(c, mt, proto_client_event_finish_handler);
  }else{
    proto_client_set_event_handler(c, mt, proto_client_event_null_handler);
  }
  *ch = c;
  return 1;
}
char
proto_client_connect(Proto_Client_Handle ch, char *host, PortType port, char* boardInit)
{
  Proto_Client *c = (Proto_Client *)ch;

  if (net_setup_connection(&(c->rpc_session.fd), host, port)<0) 
    return 'F';

  if (net_setup_connection(&(c->event_session.fd), host, port+1)<0) 
    return 'F'; 

  if (pthread_create(&(c->EventHandlerTid), NULL, 
		     &proto_client_event_dispatcher, c) !=0) {
    fprintf(stderr, 
	    "proto_client_init: create EventHandler thread failed\n");
    perror("proto_client_init:");
    return 'F';
  }

  char rc = proto_client_conn(ch, boardInit);
  PLAYER_INFO_GLOBALS.player_type = rc;
  return rc;
}

static void
marshall_mtonly(Proto_Session *s, Proto_Msg_Types mt) {
  Proto_Msg_Hdr h;
  
  bzero(&h, sizeof(h));
  h.type = mt;
  proto_session_hdr_marshall(s, &h);
};

static char
do_connect_rpc(Proto_Client_Handle ch, Proto_Msg_Types mt, char* boardInit)
{
  char rc;
  //char board[9];
  Proto_Session *s;
  Proto_Client *c = ch;
  Proto_Msg_Hdr hdr;

  s = proto_client_rpc_session(c); 

  marshall_mtonly(s, mt);
  rc = proto_session_rpc(s);//perform our rpc call
  if (rc==1) {
    proto_session_hdr_unmarshall(s, &hdr);
    rc = (char)hdr.pstate.v3.raw;
    proto_session_body_unmarshall_bytes(s, 0, 9, boardInit);
  } else {
    c->session_lost_handler(s);
    close(s->fd);
  }
  //char rcAndBoard[10];
  //rcAndBoard[0] = rc;
  //memcpy(rcAndBoard+1, &board, sizeof(board));
  return rc;
}

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
    proto_session_body_unmarshall_char(s, 0, &rc); 
  } else {
    //ADD CODE send_msg communication failed so assign the session lost handler and close the session. -JG
    c->session_lost_handler(s);
    close(s->fd);
  }
  
  return rc;
}

static int
do_mark_rpc_handler(Proto_Client_Handle ch, Proto_Msg_Types mt, int mark, char player){
	int rc;
	Proto_Session *s;
	Proto_Client *c = (Proto_Client *)ch;
	Proto_Msg_Hdr h;
	
	s = proto_client_rpc_session(c);
	bzero(&h, sizeof(s));
	h.type = mt;
	h.pstate.v0.raw = player;
	proto_session_body_marshall_int(s, mark);
	proto_session_hdr_marshall(s, &h);
	rc = proto_session_rpc(s);
	bzero(&h, sizeof(s));
	if (rc==1) {
		proto_session_hdr_unmarshall(s, &h);
		if (h.type == PROTO_MT_REP_BASE_NOT_STARTED) return 0;
		if (h.type == PROTO_MT_REP_BASE_MOVE) return 1;
		if (h.type == PROTO_MT_REP_BASE_INVALID_MOVE) return 2;
    		if (h.type == PROTO_MT_REP_BASE_NOT_TURN) return 3;
  	} else {
    		c->session_lost_handler(s);
    		close(s->fd);
  	}
	return rc;

}

static int
do_print_board_rpc_handler(Proto_Client_Handle ch, Proto_Msg_Types mt){
        int rc;
        char board[9];

	Proto_Session *s;
        Proto_Client *c = (Proto_Client *)ch;
        Proto_Msg_Hdr h;

        s = proto_client_rpc_session(c);
        
	marshall_mtonly(s, mt);

	rc = proto_session_rpc(s);
        if (rc==1) {
        	proto_session_body_unmarshall_bytes(s, 0, sizeof(board), &board);
		printGameBoard(&board);
	} else {
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

extern char proto_client_conn(Proto_Client_Handle ch, char* boardInit){
  return do_connect_rpc(ch,PROTO_MT_REQ_BASE_CONNECT, boardInit);  
}

extern int proto_client_disconnect(Proto_Client_Handle ch, char *host, PortType port){
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_DISCONNECT);  
}

extern int 
proto_client_mark(Proto_Client_Handle ch, int data, char player)
{
  return do_mark_rpc_handler(ch,PROTO_MT_REQ_BASE_MOVE, data, player);  
}

extern int
proto_client_print_board(Proto_Client_Handle ch)
{
  return do_print_board_rpc_handler(ch,PROTO_MT_REQ_BASE_PRINT);
}

extern int 
proto_client_goodbye(Proto_Client_Handle ch)
{
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_GOODBYE);  
}

void
printGameBoardFromEvent(char* board)
{
  fprintf(stderr, "\n%c|%c|%c", board[0], board[1], board[2]);
  fprintf(stderr, "\n-----");
  fprintf(stderr, "\n%c|%c|%c", board[3], board[4], board[5]);
  fprintf(stderr, "\n-----");
  fprintf(stderr, "\n%c|%c|%c", board[6], board[7], board[8]);
}

void
printGameBoard(char* board)
{
  printf("\n%c|%c|%c", board[0], board[1], board[2]);
  printf("\n-----");
  printf("\n%c|%c|%c", board[3], board[4], board[5]);
  printf("\n-----");
  printf("\n%c|%c|%c", board[6], board[7], board[8]);
}

void
printMarker()
{
 fprintf(stderr, "\n%c>", PLAYER_INFO_GLOBALS.player_type);
}
