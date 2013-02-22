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
#include <poll.h>
#include "../lib/types.h"
#include "../lib/protocol_server.h"
#include "../lib/protocol_utils.h"
struct {
        char board[9];
        char blankBoard[9];
        char curTurn;
        int IsGameStarted;
}GameBoard;
int init_Game_Board(void){
        GameBoard.board[0] = '1';
        GameBoard.board[1] = '2';
        GameBoard.board[2] = '3';
        GameBoard.board[3] = '4';
        GameBoard.board[4] = '5';
        GameBoard.board[5] = '6';
        GameBoard.board[6] = '7';
        GameBoard.board[7] = '8';
        GameBoard.board[8] = '9';
        GameBoard.blankBoard[0] = '1';
        GameBoard.blankBoard[1] = '2';
        GameBoard.blankBoard[2] = '3';
        GameBoard.blankBoard[3] = '4';
        GameBoard.blankBoard[4] = '5';
        GameBoard.blankBoard[5] = '6';
        GameBoard.blankBoard[6] = '7';
        GameBoard.blankBoard[7] = '8';
        GameBoard.blankBoard[8] = '9';
        GameBoard.curTurn = 'X';
        GameBoard.IsGameStarted = 0;
        return 1;
}
char *
getBoard(void){
	return GameBoard.board;
}
int
check_for_win(int pos){
        int player = GameBoard.curTurn;
        int numFilled = 0;
        int i = 0;
        //check for draw
        for (i = 0; i < 9; i++){
                if (GameBoard.board[i] != GameBoard.blankBoard[i]){
                        numFilled++;
                }
        }
        if (numFilled == 9) {
                return 2;
        }
        //check col
        for (i = 0; i < 9; i+=3){
                if (GameBoard.board[i+(pos%3)] != player){
                        break;
                }
                if(i == (pos%3)+6){
                        return 1;
                }
        }
        //check row
        for (i = 0; i < 3; i++){
                if(GameBoard.board[pos-(pos%3)+i] != player){
                        break;
                }
                if(i == (pos%3)+3){
                        return 1;
                }
        }
        //check diags
        if (pos%2 == 0){
                for(i = 0; i < 9; i+= 4){
                        if (GameBoard.board[i] != player){
                                break;
                        }
                        if (i == 8){
                                return 1;
                        }
                }
                for(i = 2; i < 9; i += 2){
                        if (GameBoard.board[i] != player){
                                break;
                        }
                        if (i == 6){
                                return 1;
                        }
                }
        }
        //if no wins detected, return 0
        return 0;
}
int checkMark(int marked_pos){
	if (GameBoard.board[marked_pos] == GameBoard.blankBoard[marked_pos]){
		return 0;
	}else{
		return 1;
	}
}
void setMark(int marked_pos, char player){
	GameBoard.board[marked_pos] = player;
}
char
curTurn(void){
	return GameBoard.curTurn;
}
void setCurTurn(char toSet){
	GameBoard.curTurn = toSet;
}
int
IsGameStarted(void){
	return GameBoard.IsGameStarted;
}
void
startGame(void){
	GameBoard.IsGameStarted = 1;
}
void
stopGame(void){
        GameBoard.IsGameStarted = 0;
}

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

  switch (cmd) {
  case 'd':
    proto_debug_on();
    break;
  case 'D':
    proto_debug_off();
    break;
  case 'u':
    rc = doUpdateClients();
    break;
  case 'q':
    rc=-1;
    break;
  case '\n':
  case ' ':
    rc=1;
    break;
  default:
    printf("Unkown Command\n");
  }
  return rc;
}

int
prompt(int menu) 
{
  int ret;
  int c=0;

  if (menu) printf("%s:", MenuString);
  fflush(stdout);
  c=getchar();;
  return c;
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
	init_Game_Board();
  fprintf(stderr, "RPC Port: %d, Event Port: %d\n", proto_server_rpcport(), 
	  proto_server_eventport());

  if (proto_server_start_rpc_loop()<0) {
    fprintf(stderr, "ERROR: failed to start rpc loop\n");
    exit(-1);
  }
    
  shell(NULL);

  return 0;
}
