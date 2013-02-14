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

#include "net.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_session.h"

extern void
proto_session_dump(Proto_Session *s)
{
  fprintf(stderr, "Session s=%p:\n", s);
  fprintf(stderr, " fd=%d, extra=%p slen=%d, rlen=%d\n shdr:\n  ", 
	  s->fd, s->extra,
	  s->slen, s->rlen);
  proto_dump_msghdr(&(s->shdr));
  fprintf(stderr, " rhdr:\n  ");
  proto_dump_msghdr(&(s->rhdr));
}

extern void
proto_session_init(Proto_Session *s)
{
  if (s) bzero(s, sizeof(Proto_Session));
}

extern void
proto_session_reset_send(Proto_Session *s)
{
  bzero(&s->shdr, sizeof(s->shdr));
  s->slen = 0;
}

extern void
proto_session_reset_receive(Proto_Session *s)
{
  bzero(&s->rhdr, sizeof(s->rhdr));
  s->rlen = 0;
}

static void
proto_session_hdr_marshall_sver(Proto_Session *s, Proto_StateVersion v)
{
	if (proto_session_body_marshall_ll(s, v.raw) >= 0) {;//given this function to marshall data into buffer to send. My main concern is that the original version of this code didn't need the proto_session_body_marshall_ll function (there was no ADD CODE), but I thought it was necessary -WA
  		s->shdr.sver.raw = htonll(v.raw);
	}
}

static void
proto_session_hdr_unmarshall_sver(Proto_Session *s, Proto_StateVersion *v)
{
  	 if (proto_session_body_unmarshall_ll(s, someoffsett, &v.raw) >= 0) {;//given this function to unmarshall data from recieved buffer, need to figure out how to calculate offset -WA
	v.raw = ntohll(s->rhdr.sver.raw);
}

static void
proto_session_hdr_marshall_pstate(Proto_Session *s, Proto_Player_State *ps)
{	
	//so my understanding of this is: whenever we want do an rpc, we marshall the data to be sent. 
	//This data is stored in the sbuf, which is then written to whatever socket
	//the recieving end then unmarshalls the data into the datastructures.
	//the important thing is that sbuf and rbuf are being written to whenever is sending or reading respectively
	proto_session_body_marshall_int(s, ps->v0.raw)
        proto_session_body_marshall_int(s, ps->v1.raw)
        proto_session_body_marshall_int(s, ps->v2.raw) 
        proto_session_body_marshall_int(s, ps->v3.raw)
    	s->shdr.pstate.v0.raw  = htonl(ps->v0.raw);
    	s->shdr.pstate.v1.raw  = htonl(ps->v1.raw);
	s->shdr.pstate.v2.raw  = htonl(ps->v2.raw);
	s->shdr.pstate.v3.raw  = htonl(ps->v3.raw);

}

static void
proto_session_hdr_unmarshall_pstate(Proto_Session *s, Proto_Player_State *ps)
{	
	proto_session_body_unmarshall_int(s, someoffset, &s->rhdr.pstate.v0.raw) 
        proto_session_body_unmarshall_int(s, someoffset, &s->rhdr.pstate.v1.raw) 
	proto_session_body_unmarshall_int(s, someoffset, &s->rhdr.pstate.v2.raw) 
        proto_session_body_unmarshall_int(s, someoffset, &s->rhdr.pstate.v3.raw)

	ps->v0.raw = s->rhdr.pstate.v0.raw;
	ps->v1.raw = s->rhdr.pstate.v1.raw;
	ps->v2.raw = s->rhdr.pstate.v2.raw;
	ps->v3.raw = s->rhdr.pstate.v3.raw;
}

static void
proto_session_hdr_marshall_gstate(Proto_Session *s, Proto_Game_State *gs)
{
	proto_session_body_marshall_int(s, gs->v0.raw)
        proto_session_body_marshall_int(s, gs->v1.raw)
        proto_session_body_marshall_int(s, gs->v2.raw)
        s->shdr.gstate.v0.raw  = htonl(gs->v0.raw);
        s->shdr.gstate.v1.raw  = htonl(gs->v1.raw);
        s->shdr.gstate.v2.raw  = htonl(gs->v2.raw);

}

static void
proto_session_hdr_unmarshall_gstate(Proto_Session *s, Proto_Game_State *gs)
{
	proto_session_body_unmarshall_int(s, someoffset, &s->rhdr.gstate.v0.raw) 
        proto_session_body_unmarshall_int(s, someoffset, &s->rhdr.gstate.v1.raw)
        proto_session_body_unmarshall_int(s, someoffset, &s->rhdr.gstate.v2.raw)

        gs->v0.raw = s->rhdr.gstate.v0.raw;
        gs->v1.raw = s->rhdr.gstate.v1.raw;
        gs->v2.raw = s->rhdr.gstate.v2.raw;
        gs->v3.raw = s->rhdr.gstate.v3.raw;
}
static int
proto_session_hdr_unmarshall_blen(Proto_Session *s)
{

  proto_session_body_unmarshall_int(s, someoffset, &s->rhdr.blen);
	
}

static void
proto_session_hdr_marshall_type(Proto_Session *s, Proto_Msg_Types t)
{
  ADD CODE
}

static int
proto_session_hdr_unmarshall_version(Proto_Session *s)
{
  ADD CODE 
}

extern Proto_Msg_Types
proto_session_hdr_unmarshall_type(Proto_Session *s)
{
  ADD CODE 
}

extern void
proto_session_hdr_unmarshall(Proto_Session *s, Proto_Msg_Hdr *h)
{
  
  h->version = proto_session_hdr_unmarshall_version(s);
  h->type = proto_session_hdr_unmarshall_type(s);
  proto_session_hdr_unmarshall_sver(s, &h->sver);
  proto_session_hdr_unmarshall_pstate(s, &h->pstate);
  proto_session_hdr_unmarshall_gstate(s, &h->gstate);
  h->blen = proto_session_hdr_unmarshall_blen(s);
}
   
extern void
proto_session_hdr_marshall(Proto_Session *s, Proto_Msg_Hdr *h)
{
  // ignore the version number and hard code to the version we support
  s->shdr.version = PROTOCOL_BASE_VERSION;
  proto_session_hdr_marshall_type(s, h->type);
  proto_session_hdr_marshall_sver(s, h->sver);
  proto_session_hdr_marshall_pstate(s, &h->pstate);
  proto_session_hdr_marshall_gstate(s, &h->gstate);
  // we ignore the body length as we will explicity set it
  // on the send path to the amount of body data that was
  // marshalled.
}

extern int 
proto_session_body_marshall_ll(Proto_Session *s, long long v)
{
  if (s && ((s->slen + sizeof(long long)) < PROTO_SESSION_BUF_SIZE)) {
    *((int *)(s->sbuf + s->slen)) = htonll(v);
    s->slen+=sizeof(long long);
    return 1;
  }
  return -1;
}

extern int 
proto_session_body_unmarshall_ll(Proto_Session *s, int offset, long long *v)
{
  if (s && ((s->rlen - (offset + sizeof(long long))) >=0 )) {
    *v = *((long long *)(s->rbuf + offset));
    *v = htonl(*v);
    return offset + sizeof(long long);
  }
  return -1;
}

extern int 
proto_session_body_marshall_int(Proto_Session *s, int v)
{
  if (s && ((s->slen + sizeof(int)) < PROTO_SESSION_BUF_SIZE)) {
    *((int *)(s->sbuf + s->slen)) = htonl(v);
    s->slen+=sizeof(int);
    return 1;
  }
  return -1;
}

extern int 
proto_session_body_unmarshall_int(Proto_Session *s, int offset, int *v)
{
  if (s && ((s->rlen  - (offset + sizeof(int))) >=0 )) {
    *v = *((int *)(s->rbuf + offset));
    *v = htonl(*v);
    return offset + sizeof(int);
  }
  return -1;
}

extern int 
proto_session_body_marshall_char(Proto_Session *s, char v)
{
  if (s && ((s->slen + sizeof(char)) < PROTO_SESSION_BUF_SIZE)) {
    s->sbuf[s->slen] = v;
    s->slen+=sizeof(char);
    return 1;
  }
  return -1;
}

extern int 
proto_session_body_unmarshall_char(Proto_Session *s, int offset, char *v)
{
  if (s && ((s->rlen - (offset + sizeof(char))) >= 0)) {
    *v = s->rbuf[offset];
    return offset + sizeof(char);
  }
  return -1;
}

extern int
proto_session_body_reserve_space(Proto_Session *s, int num, char **space)
{
  if (s && ((s->slen + num) < PROTO_SESSION_BUF_SIZE)) {
    *space = &(s->sbuf[s->slen]);
    s->slen += num;
    return 1;
  }
  *space = NULL;
  return -1;
}

extern int
proto_session_body_ptr(Proto_Session *s, int offset, char **ptr)
{
  if (s && ((s->rlen - offset) > 0)) {
    *ptr = &(s->rbuf[offset]);
    return 1;
  }
  return -1;
}
	    
extern int
proto_session_body_marshall_bytes(Proto_Session *s, int len, char *data)
{
  if (s && ((s->slen + len) < PROTO_SESSION_BUF_SIZE)) {
    memcpy(s->sbuf + s->slen, data, len);
    s->slen += len;
    return 1;
  }
  return -1;
}

extern int
proto_session_body_unmarshall_bytes(Proto_Session *s, int offset, int len, 
				     char *data)
{
  if (s && ((s->rlen - (offset + len)) >= 0)) {
    memcpy(data, s->rbuf + offset, len);
    return offset + len;
  }
  return -1;
}

// rc < 0 on comm failures
// rc == 1 indicates comm success
extern  int
proto_session_send_msg(Proto_Session *s, int reset)
{
  s->shdr.blen = htonl(s->slen);

  // write request
  ADD CODE
  //here we'll need to write to the socket or something the content of sbuf
  if (proto_debug()) {
    fprintf(stderr, "%p: proto_session_send_msg: SENT:\n", pthread_self());
    proto_session_dump(s);
  }

  // communication was successfull 
  if (reset) proto_session_reset_send(s);

  return 1;
}

extern int
proto_session_rcv_msg(Proto_Session *s)
{
  
  proto_session_reset_receive(s);

  // read reply
  ADD CODE
	//we'll read from the socket the recieved bytes, which we'll store in rbuf, and then unmarshall	
  if (proto_debug()) {
    fprintf(stderr, "%p: proto_session_rcv_msg: RCVED:\n", pthread_self());
    proto_session_dump(s);
  }
  return 1;
}

extern int
proto_session_rpc(Proto_Session *s)
{
  int rc;
  
  ADD CODE
	//find some way to switch between send and rcv
	//most likely, we'll read from s->shdr.types to determine what the appropriate action to take is.
  return rc;
}

