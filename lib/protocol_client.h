#ifndef __DAGAME_PROTOCOL_CLIENT_H__
#define __DAGAME_PROTOCOL_CLIENT_H__
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

#include "net.h"
#include "protocol.h"
#include "protocol_session.h"
#include "misc.h"
//#include "maze.h"

typedef void * Proto_Client_Handle;

extern Proto_Session *proto_client_rpc_session(Proto_Client_Handle ch);
extern Proto_Session *proto_client_event_session(Proto_Client_Handle ch);

extern int proto_client_init(Proto_Client_Handle *ch);
extern int proto_client_connect(Proto_Client_Handle ch, char *host, PortType p, int *id, int *team, int *team_color, Tuple *pos);

extern int proto_client_set_session_lost_handler(Proto_Client_Handle ch,
						 Proto_MT_Handler h);

extern int proto_client_set_event_handler(Proto_Client_Handle ch,
					  Proto_Msg_Types mt,
					  Proto_MT_Handler h);

static char proto_client_event_conn_handler(Proto_Session *s);

// client side protocol rpc's
extern int proto_client_hello(Proto_Client_Handle ch, int *id, int *team, int *team_color, Tuple *pos);
//extern int proto_client_map_info(Proto_Client_Handle ch, Pos *tuple);
//extern int proto_client_map_info_team_1(Proto_Client_Handle ch, Pos *tuple);
//extern int proto_client_map_info_team_2(Proto_Client_Handle ch, Pos *tuple);
//extern int proto_client_map_dim(Proto_Client_Handle ch, Pos *dim);
//extern int proto_client_map_dump(Proto_Client_Handle ch);
//extern int proto_client_map_cinfo(Proto_Client_Handle ch, Pos *pos, Cell_Type *cell_type, int *team, int *occupied);
//extern int proto_client_move(Proto_Client_Handle ch, char d);
extern int proto_client_goodbye(Proto_Client_Handle ch);
#endif
