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
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include "net.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_server.h"
#include "misc.h"
#include "maze.h"
#define PROTO_SERVER_MAX_EVENT_SUBSCRIBERS 1024

struct {
  FDType   RPCListenFD;
  PortType RPCPort;


  FDType             EventListenFD;
  PortType           EventPort;
  pthread_t          EventListenTid;
  pthread_mutex_t    EventSubscribersLock;
  int                EventLastSubscriber;
  int                EventNumSubscribers;
  FDType             EventSubscribers[PROTO_SERVER_MAX_EVENT_SUBSCRIBERS];
  Proto_Session      EventSession; 
  pthread_t          RPCListenTid;
  Proto_MT_Handler   session_lost_handler;
  Proto_MT_Handler   base_req_handlers[PROTO_MT_REQ_BASE_RESERVED_LAST - 
				       PROTO_MT_REQ_BASE_RESERVED_FIRST-1];
} Proto_Server;

Map game_map;

extern PortType proto_server_rpcport(void) { return Proto_Server.RPCPort; }
extern PortType proto_server_eventport(void) { return Proto_Server.EventPort; }
extern Proto_Session *
proto_server_event_session(void) 
{ 
  return &Proto_Server.EventSession; 
}

extern int
proto_server_set_session_lost_handler(Proto_MT_Handler h)
{
  Proto_Server.session_lost_handler = h;
}

extern int
proto_server_set_req_handler(Proto_Msg_Types mt, Proto_MT_Handler h)
{
  int i;

  if (mt>PROTO_MT_REQ_BASE_RESERVED_FIRST &&
      mt<PROTO_MT_REQ_BASE_RESERVED_LAST) {
    i = mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
    
    //ADD CODE: Adding the set handle code with the correct handler index -RC
    Proto_Server.base_req_handlers[i] = h; 
    return 1;
  } else {
    return -1;
  }
}


static int
proto_server_record_event_subscriber(int fd, int *num)
{
  int rc=-1;

  pthread_mutex_lock(&Proto_Server.EventSubscribersLock);

  //Goes through and sees if this is the first subscriber -RC
  if (Proto_Server.EventLastSubscriber < PROTO_SERVER_MAX_EVENT_SUBSCRIBERS
      && Proto_Server.EventSubscribers[Proto_Server.EventLastSubscriber]
      ==-1) {
    //ADD CODE: Set the first subscriber -RC
    //Proto_Server.EventLastSubscriber = 0; 
    Proto_Server.EventNumSubscribers++;
    Proto_Server.EventSubscribers[Proto_Server.EventLastSubscriber] = fd;
    rc = 1;
  } else {
    int i;
    for (i=0; i< PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
      if (Proto_Server.EventSubscribers[i]==-1) {
	//ADD CODE: Set the last subscriber  -RC
        Proto_Server.EventLastSubscriber = i;
	Proto_Server.EventSubscribers[i] = fd;
	Proto_Server.EventNumSubscribers++;
        *num=i;
	i = PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; //BREAK OUT -RC
	rc=1;
      }
    }
  }

  pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);

  return rc;
}

static
void *
proto_server_event_listen(void *arg)
{
  int fd = Proto_Server.EventListenFD;
  int connfd;

  if (net_listen(fd)<0) {
    exit(-1);
  }

  for (;;) {
    connfd = net_accept(fd); //ADD CODE:  Accept the event listener here - RC
    if (connfd < 0) {
      fprintf(stderr, "Error: EventListen accept failed (%d)\n", errno);
    } else {
      int i;
      fprintf(stderr, "EventListen: connfd=%d -> ", connfd);

      /*Below ADD CODE: I call the function event subscriber to add a new subscriber -RC*/
      if (proto_server_record_event_subscriber(connfd, &i)<0) {
	fprintf(stderr, "oops no space for any more event subscribers\n");
	close(connfd);
      } else {
	fprintf(stderr, "subscriber num %d\n", i);
      }
    } 
  }
} 

void
proto_server_post_event(void) 
{
  int i;
  int num;
  //ADDED CODE
  struct timeval timeout;//timeout struct -WA
  int ready;//ready int to be used later -WA
  fd_set   fdset;//fdset needed by select function -WA

  timeout.tv_sec = 3;//set the time to 3 seconds; seems reasonable -WA
  timeout.tv_usec = 0;
  //END ADDED CODE

  pthread_mutex_lock(&Proto_Server.EventSubscribersLock);

  i = 0;
  num = Proto_Server.EventNumSubscribers;
  while (num) {
    Proto_Server.EventSession.fd = Proto_Server.EventSubscribers[i];
    if (Proto_Server.EventSession.fd != -1) {
	//fprintf(stderr, "fd=%d\n", Proto_Server.EventSession.fd);
      num--;
	//ADDED CODE -WA
      if (proto_session_send_msg(&Proto_Server.EventSession, 0)<0) {//here we push to the client the updated state -WA
	//END ADDED CODE
	// must have lost an event connection
	close(Proto_Server.EventSession.fd+1);
	Proto_Server.EventSubscribers[i]=-1;
	Proto_Server.EventNumSubscribers--;
	Proto_Server.session_lost_handler(&Proto_Server.EventSession);
	//Proto_Server.ADD CODE
      } else {
	//ADDED CODE -WA
	//fprintf(stderr, "message was sent\n");
      	FD_ZERO(&fdset);//zero the set
      	FD_SET(Proto_Server.EventSession.fd, &fdset);//set it to the given FD_Type
      	ready = select(Proto_Server.EventSession.fd+1, &fdset, NULL, NULL, &timeout);
	//select function will see if there is anything to read
	//if after timeout, there is nothing, it returns -1
     	if (ready > 0){
         	proto_session_rcv_msg(&Proto_Server.EventSession);//recieve acknowledgement from server
		//we'll need an additonal check to make sure the client isn't just sending back
		//garbage; maybe check to see if header sent is the same as header recieved
		//or check in the body for "ACK". -WA
      	}else{
		close(Proto_Server.EventSession.fd);
        	Proto_Server.EventSubscribers[i]=-1;
        	Proto_Server.EventNumSubscribers--;
		Proto_Server.session_lost_handler(&Proto_Server.EventSession);
        	//Proto_Server.ADD CODE
	}
      }
      //END ADDED CODE -WA
      // FIXME: add ack message here to ensure that game is updated 
      // correctly everywhere... at the risk of making server dependent
      // on client behaviour  (use time out to limit impact... drop
      // clients that misbehave but be carefull of introducing deadlocks
	
    }
    i++;
  }
  proto_session_reset_send(&Proto_Server.EventSession);
  pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
}


static void *
proto_server_req_dispatcher(void * arg)
{
  Proto_Session s;
  Proto_Msg_Types mt;
  Proto_MT_Handler hdlr;
  int i;
  unsigned long arg_value = (unsigned long) arg;
  
  pthread_detach(pthread_self());

  proto_session_init(&s);

  s.fd = (FDType) arg_value;

  fprintf(stderr, "proto_rpc_dispatcher: %p: Started: fd=%d\n", 
	  pthread_self(), s.fd);

  for (;;) {
    if (proto_session_rcv_msg(&s)==1) {
        //ADD CODE: Very similar to the dispatcher from client - RC
        mt = proto_session_hdr_unmarshall_type(&s);
        if(mt > PROTO_MT_REQ_BASE_RESERVED_FIRST &&
           mt < PROTO_MT_REQ_BASE_RESERVED_LAST) { // Changed PROTO_MT_EVENT_BASE_RESERVED_FIRST and LAST to REQ, since we are dealing with requests/rpc and not the event channel. -JG
        i=mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
        hdlr = Proto_Server.base_req_handlers[i]; 
	if (hdlr(&s)<0) goto leave;
      }
    } else {
      goto leave;
    }
  }
 leave:
  //Proto_Server.ADD CODE Not sure but I think that we need to nullify with a -1 the eventsession.fd because we close the session next -RC
  Proto_Server.EventSession.fd = -1;
  close(s.fd);
  return NULL;
}

static
void *
proto_server_rpc_listen(void *arg)
{
  int fd = Proto_Server.RPCListenFD;
  unsigned long connfd;
  pthread_t tid;
  
  if (net_listen(fd) < 0) {
    fprintf(stderr, "Error: proto_server_rpc_listen listen failed (%d)\n", errno);
    exit(-1);
  }

  for (;;) {
    connfd = net_accept(fd); //ADD CODE This is accepting the connection instead of establishing one I beleive -RC
    if (connfd < 0) {
      fprintf(stderr, "Error: proto_server_rpc_listen accept failed (%d)\n", errno);
    } else {
      pthread_create(&tid, NULL, &proto_server_req_dispatcher,
		     (void *)connfd);
    }
  }
}

extern int
proto_server_start_rpc_loop(void)
{
  if (pthread_create(&(Proto_Server.RPCListenTid), NULL, 
		     &proto_server_rpc_listen, NULL) !=0) {
    fprintf(stderr, 
	    "proto_server_rpc_listen: pthread_create: create RPCListen thread failed\n");
    perror("pthread_create:");
    return -3;
  }
  return 1;
}

static int 
proto_session_lost_default_handler(Proto_Session *s)
{
  fprintf(stderr, "Session lost...:\n");
  proto_session_dump(s);
  return -1;
}

static int 
proto_server_mt_null_handler(Proto_Session *s)
{
  int rc=1;
  Proto_Msg_Hdr h;
  
  fprintf(stderr, "proto_server_mt_null_handler: invoked for session:\n");
  proto_session_dump(s);

  // setup dummy reply header : set correct reply message type and 
  // everything else empty
  bzero(&h, sizeof(s));
  h.type = proto_session_hdr_unmarshall_type(s);
  h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;
  proto_session_hdr_marshall(s, &h);

  // setup a dummy body that just has a return code 
  proto_session_body_marshall_int(s, 0xdeadbeef);
  rc=proto_session_send_msg(s,1);

  return rc;
}

/* Handler for Dumping Map in ASCII */
static int 
proto_server_mt_dump_handler(Proto_Session *s){
  int rc = 1;
  Proto_Msg_Hdr h;

  fprintf(stderr, "proto_server_mt_hello_handler: invoked for session:\n");
  proto_session_dump(s);
  bzero(&h, sizeof(s));
  h.type = proto_session_hdr_unmarshall_type(s);
  h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;

  proto_session_hdr_marshall(s, &h);
  
  //Dump out ASCII of map
  char* map_data = (char*)dump_map(getMap());
  fprintf(stderr, "%s", map_data);
 
  rc=proto_session_send_msg(s,1);
  
  return rc;
}

/* Handler for returning num_home and num_jail */
static int
proto_server_mt_map_info_team_handler(Proto_Session *s){
  int rc = 1;
  Proto_Msg_Hdr h;
  
  int home_cells=0;
  int jail_cells=0;
  Color c;

  fprintf(stderr, "proto_server_mt_map_info_team_handler: invoked for session:\n");
  proto_session_dump(s);
  bzero(&h, sizeof(s));
  h.type = proto_session_hdr_unmarshall_type(s);

  if(h.type == PROTO_MT_REQ_BASE_MAP_INFO_1){
    c = RED;
  }else if(h.type == PROTO_MT_REQ_BASE_MAP_INFO_2){
    c = GREEN;
  }

  home_cells = num_home(c, getMap());
  jail_cells = num_jail(c, getMap());

  h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;
  proto_session_body_marshall_int(s, home_cells);
  proto_session_body_marshall_int(s, jail_cells);

  proto_session_hdr_marshall(s, &h);
  
  rc=proto_session_send_msg(s,1);

  return rc;
}

/* Handler for returning num_walls and num_floor */
static int
proto_server_mt_map_info_handler(Proto_Session *s){
  int rc = 1;
  Proto_Msg_Hdr h;

  int wall_cells=0;
  int floor_cells=0;

  fprintf(stderr, "proto_server_mt_map_info_handler: invoked for session:\n");
  proto_session_dump(s);
  bzero(&h, sizeof(s));
  h.type = proto_session_hdr_unmarshall_type(s);

  wall_cells = num_wall(getMap());
  floor_cells = num_floor(getMap());

  h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;
  proto_session_hdr_marshall(s, &h);
  proto_session_body_marshall_int(s, wall_cells);
  proto_session_body_marshall_int(s, floor_cells);

  rc=proto_session_send_msg(s,1);

  return rc;
}

/* Handler for returning dim */
static int
proto_server_mt_dim_handler(Proto_Session *s){
  int rc = 1;
  Proto_Msg_Hdr h;
  Pos dimensions;

  fprintf(stderr, "proto_server_mt_dim_handler: invoked for session:\n");
  proto_session_dump(s);
  bzero(&h, sizeof(s));
  h.type = proto_session_hdr_unmarshall_type(s);

  h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;
  proto_session_hdr_marshall(s, &h);
  
  dim(getMap(), &dimensions);
  int x = dimensions.x;
  int y = dimensions.y;
  proto_session_body_marshall_int(s, x);
  proto_session_body_marshall_int(s, y);

  rc=proto_session_send_msg(s,1);

  return rc;
}

/* Handler for returning cinfo */
static int
proto_server_mt_cinfo_handler(Proto_Session *s){
  int rc = 1;
  Proto_Msg_Hdr h;
  Cell_Type t;
  int team=0;
  int occupied=0;
  Cell cell_xy;  

  int x;
  int y;  

  fprintf(stderr, "proto_server_mt_dim_handler: invoked for session:\n");
  proto_session_dump(s);
  bzero(&h, sizeof(s));
  h.type = proto_session_hdr_unmarshall_type(s);
  h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;

  proto_session_body_unmarshall_int(s, 0, &x);
  proto_session_body_unmarshall_int(s, sizeof(int), &y);

  cinfo(getMap(), &cell_xy, x, y);
  
  if(cell_xy.t == -1 || cell_xy.c == -1){
    t = -1;
    team = -1;
    occupied = -1;
  }else{
    t = cell_xy.t;

    if(cell_xy.c == RED){
      team = 1;
    }else{
      team = 2;
    }

    if(&cell_xy.hammer || &cell_xy.flag){
      occupied = 1;
    }else{
      occupied = 0;
    }
  }

  proto_session_hdr_marshall(s, &h);
  
  proto_session_body_marshall_int(s, t);
  proto_session_body_marshall_int(s, team);
  proto_session_body_marshall_int(s, occupied);

  rc=proto_session_send_msg(s,1);

  return rc;
}

/* Handler for Connection */
static int
proto_server_mt_hello_handler(Proto_Session *s){
  int rc = 1;
  Proto_Msg_Hdr h;

  fprintf(stderr, "proto_server_mt_hello_handler: invoked for session:\n");
  proto_session_dump(s);
  
  //pthread_mutex_lock(&Proto_Server.EventSubscribersLock);
  //int subscribers = Proto_Server.EventNumSubscribers;
  
  bzero(&h, sizeof(s));
  h.type = proto_session_hdr_unmarshall_type(s);
  h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;

  proto_session_hdr_marshall(s, &h);  
  rc=proto_session_send_msg(s,1);
  
  //pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
   
  return rc;
}

/* Handler for Disconnect */
static int
proto_server_mt_goodbye_handler(Proto_Session *s){
  int rc = 1;
  Proto_Msg_Hdr h;

  fprintf(stderr, "proto_server_mt_goodbye_handler: invoked for session:\n");
  proto_session_dump(s);

  bzero(&h, sizeof(s));
  h.type = proto_session_hdr_unmarshall_type(s);
  h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;
  proto_session_hdr_marshall(s, &h);

  int userfd = s->fd;
  int i;
  
  pthread_mutex_lock(&Proto_Server.EventSubscribersLock);
 fprintf(stderr, "looking for %d", userfd+1);
  for (i=0; i< PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
    if(Proto_Server.EventSubscribers[i] == userfd-1){
      Proto_Server.EventSession.fd = Proto_Server.EventSubscribers[i];
      Proto_Server.EventSubscribers[i] = -1;
      Proto_Server.EventNumSubscribers--;
      //Proto_Server.EventLastSubscriber = (i==0 ? 0 : i-1);
      fprintf(stderr, "disconnected from %d", Proto_Server.EventSession.fd);
      close(Proto_Server.EventSession.fd);
      close(Proto_Server.EventSession.fd+1);
      Proto_Server.session_lost_handler(&Proto_Server.EventSession);
      //stopGame();
      //close(userfd);
      break;
    }
  }
  
  pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);

  Proto_Session *se;
  Proto_Msg_Hdr hdr;
  
  proto_session_body_marshall_int(s, 1);
  rc=proto_session_send_msg(s,1);
    
  //Post Event Disconnect 
  se = proto_server_event_session();
  hdr.type = PROTO_MT_EVENT_BASE_GOODBYE;
  proto_session_body_marshall_int(se, i);
  proto_session_hdr_marshall(se, &hdr);
  proto_server_post_event(); 

  return rc;
}

/*/extern void
proto_server_not_started_handler(Proto_Session *s, char player, char *board, int IsStarted){
        prepare_for_post(s, player, IsStarted, board, PROTO_MT_REP_BASE_NOT_STARTED);
        proto_session_send_msg(s, 1);
}*/

extern int
proto_server_init(void)
{
  int i;
  int rc;

  proto_session_init(&Proto_Server.EventSession);

  proto_server_set_session_lost_handler(
				     proto_session_lost_default_handler);
  for (i=PROTO_MT_REQ_BASE_RESERVED_FIRST+1; 
       i<PROTO_MT_REQ_BASE_RESERVED_LAST; i++) {
    //ADD CODE: Looping through the req's and setting them to the null handler -RC
    if(i == PROTO_MT_REQ_BASE_HELLO){
      proto_server_set_req_handler(i, proto_server_mt_hello_handler);
    }else if(i == PROTO_MT_REQ_BASE_GOODBYE){
      proto_server_set_req_handler(i, proto_server_mt_goodbye_handler);
    }else if(i == PROTO_MT_REQ_BASE_MAP_DUMP){
      proto_server_set_req_handler(i, proto_server_mt_dump_handler);
    }else if(i == PROTO_MT_REQ_BASE_MAP_INFO_1 || i == PROTO_MT_REQ_BASE_MAP_INFO_2){
      proto_server_set_req_handler(i, proto_server_mt_map_info_team_handler);
    }else if(i == PROTO_MT_REQ_BASE_MAP_INFO){
      proto_server_set_req_handler(i, proto_server_mt_map_info_handler);
    }else if(i == PROTO_MT_REQ_BASE_MAP_DIM){
      proto_server_set_req_handler(i, proto_server_mt_dim_handler);
    }else if(i == PROTO_MT_REQ_BASE_MAP_CINFO){
      proto_server_set_req_handler(i, proto_server_mt_cinfo_handler);
    }
  }

  for (i=0; i<PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
    Proto_Server.EventSubscribers[i]=-1;
  }
  Proto_Server.EventNumSubscribers=0;
  Proto_Server.EventLastSubscriber=0;
  pthread_mutex_init(&Proto_Server.EventSubscribersLock, 0);


  rc=net_setup_listen_socket(&(Proto_Server.RPCListenFD),
			     &(Proto_Server.RPCPort));

  if (rc==0) { 
    fprintf(stderr, "prot_server_init: net_setup_listen_socket: FAILED for RPCPort\n");
    return -1;
  }

  Proto_Server.EventPort = Proto_Server.RPCPort + 1;

  rc=net_setup_listen_socket(&(Proto_Server.EventListenFD),
			     &(Proto_Server.EventPort));

  if (rc==0) { 
    fprintf(stderr, "proto_server_init: net_setup_listen_socket: FAILED for EventPort=%d\n", 
	    Proto_Server.EventPort);
    return -2;
  }

  if (pthread_create(&(Proto_Server.EventListenTid), NULL, 
		     &proto_server_event_listen, NULL) !=0) {
    fprintf(stderr, 
	    "proto_server_init: pthread_create: create EventListen thread failed\n");
    perror("pthread_createt:");
    return -3;
  }

  return 0;
}
