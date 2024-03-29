#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

//#include "misc.h"
#include "maze.h"

Hammer* init_hammer(){
  Hammer *hammer = (Hammer *)malloc(sizeof(Hammer));
  bzero(hammer, sizeof(Hammer));
  hammer->p.x = 0;
  hammer->p.y = 0;
  hammer->charges = 1;

  return hammer;
}

int load_map(char* map_file, Map *map){
	int rc=1;
int i, j;
  map->num_floor_cells = 0;
  map->num_wall_cells = 0;
  for (j = 0; j < MAPHEIGHT; j++){
  	for (i = 0; i < MAPWIDTH; i++){
             Cell c;
	     c.hammer = NULL; c.flag = NULL; c.player = NULL;
             if (i > 99) {
               c.c = GREEN;
             }else{
               c.c = RED;
             }
                        switch(map_file[i+(j*MAPHEIGHT)]){
                                case ' ':
					map->num_floor_cells++;
                                        c.t = FLOOR;
                                        break;
                                case '#':
					map->num_wall_cells++;
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
                                case 's':
                                        c.t = FLOOR;
                                        c.c = RED;
					map->hammer_1->p.x = i;
					map->hammer_1->p.y = j;
 					c.hammer = map->hammer_1;
					break;
                                case 'S':
                                        c.t = FLOOR;
                                        c.c = GREEN;
					map->hammer_2->p.x = i;
                                        map->hammer_2->p.y = j;
                                        c.hammer = map->hammer_2;
					break;
                                default:
                                        return -1;
                        }
                        c.p.x = i;
                        c.p.y = j;
                        map->cells[i+(j*MAPHEIGHT)] = c;
			/*if (i+j == (198)){
				fprintf(stderr, "Last cell type = %d, Team = %d, x = %d, y = %d\n", c.t, c.c, c.p.x, c.p.y);
			}*/
                }
  }
  check_breakable(map);
  map->w = MAPWIDTH;
  map->h = MAPHEIGHT;
  fprintf(stderr, "Read in %d rows and %d columns\n", j, i);
  return rc;
}
int check_breakable(Map *map){
  int i, j;
  for (j = 0; j < MAPHEIGHT; j++){
	map->cells[j*MAPHEIGHT].breakable = 1;
 	map->cells[(MAPWIDTH-1)+j*MAPHEIGHT].breakable = 1;
  }
  j = 0;
  for (i = 1; i < MAPWIDTH-1; i++){
	map->cells[i].breakable = 1;
        map->cells[i+(MAPHEIGHT*(MAPHEIGHT-1))].breakable = 1;
  }
  i = 0;
  
  int m, n;
  for (j = 1; j < MAPHEIGHT-1; j++){
        for (i = 1; i < MAPWIDTH-1; i++){
		if (map->cells[i+(j*MAPHEIGHT)].t == WALL){
			map->cells[i+(j*MAPHEIGHT)].breakable = 0;
        	     for(m = j-1; m <= j+1; m++){
			for (n = i-1; n <= i+1; n++){
			  if (map->cells[n+(m*MAPHEIGHT)].t == HOME || map->cells[n+(m*MAPHEIGHT)].t == JAIL){
				map->cells[i+(j*MAPHEIGHT)].breakable = 1;
				m = j+2;
				n = i+2;
			  }
			}
		     }
		}
	}
  }
  return 0;
}

int take_hammer(Map *map, Player *player, int *numCellsToUpdate, void **cellsToUpdate){
	if (map->cells[(player->pos.x)+((player->pos.y)*MAPHEIGHT)].hammer != NULL && player->hammer == 0){
fprintf( stderr, "Cell set\n" );	
		if (map->cells[(player->pos.x)+((player->pos.y)*MAPHEIGHT)].c == RED) {
		  map->hammer_1->p.x = -1;
		  map->hammer_1->p.y = -1;
		  player->hammer = 1;
		} else {
		  map->hammer_2->p.x = -1;
                  map->hammer_2->p.y = -1;
		  player->hammer = 2;
		}
		map->cells[(player->pos.x)+((player->pos.y)*MAPHEIGHT)].hammer = NULL;
		cellsToUpdate[*numCellsToUpdate] = (void *)&map->cells[(player->pos.x)+((player->pos.y)*MAPHEIGHT)];
		(*numCellsToUpdate)++;
		player->timestamp = (unsigned)time(NULL);
		return 1;
	}else{
		return 0;
	}
}

int take_flag(Map *map, Player *player, int *numCellsToUpdate, void **cellsToUpdate){
  int x,y;
  x = player->pos.x;
  y = player->pos.y;
  if (map->cells[x+(y*MAPHEIGHT)].flag != NULL && player->flag < 1){
    fprintf( stderr, "Cell set\n" );
    if(map->cells[x+(y*MAPHEIGHT)].flag->c == RED){
      player->flag = 1;
      map->flag_red->p.x = -1;
      map->flag_red->p.y = -1;
      map->cells[x+(y*MAPHEIGHT)].flag = NULL;
      cellsToUpdate[*numCellsToUpdate] = (void *)&map->cells[x+(y*MAPHEIGHT)];
      (*numCellsToUpdate)++;
    }else if(map->cells[x+(y*MAPHEIGHT)].flag->c == GREEN){
      player->flag = 2;
      map->flag_green->p.x = -1;
      map->flag_green->p.y = -1;
      map->cells[x+(y*MAPHEIGHT)].flag = NULL;
      cellsToUpdate[*numCellsToUpdate] = (void *)&map->cells[x+(y*MAPHEIGHT)];
      (*numCellsToUpdate)++;
    }
	player->timestamp = (unsigned)time(NULL);
    return 1;
  }else{
    return 0;
  }
}
void find_free_cell(Color team_color, Cell_Type cell_type, Pos *p, Map *map){
  int j, i;
  Cell c;

  for (j = 0; j < MAPHEIGHT; j++){
    for (i = 0; i < MAPWIDTH; i++){
      c = map->cells[i+(j*MAPHEIGHT)];

      if(c.t == cell_type && c.c == team_color){ 
        if(!c.player && !c.hammer && !c.flag){
          p->x = c.p.x;
          p->y = c.p.y;
          i = MAPWIDTH;
          j = MAPHEIGHT;
        }
      }
    }
  }
}

int drop_flag(Map *map, Player *player, int *numCellsToUpdate, void **cellsToUpdate){
  int x,y;
  x = player->pos.x;
  y = player->pos.y;
  int flag_type = player->flag;

  if(flag_type != 0){
    fprintf( stderr, "Drop Flag\n" );
    if(flag_type == 1){
      player->flag = 0;
      map->flag_red->p.x = x;
      map->flag_red->p.y = y;
      map->cells[x+(y*MAPHEIGHT)].flag = map->flag_red;
      cellsToUpdate[*numCellsToUpdate] = (void *)&map->cells[x+(y*MAPHEIGHT)];
      (*numCellsToUpdate)++;
    }else if(flag_type == 2){
      player->flag = 0;
      map->flag_green->p.x = x;
      map->flag_green->p.y = y;
      map->cells[x+(y*MAPHEIGHT)].flag = map->flag_green;
      cellsToUpdate[*numCellsToUpdate] = (void *)&map->cells[x+(y*MAPHEIGHT)];
      (*numCellsToUpdate)++;
    }
	player->timestamp = (unsigned)time(NULL);
    return flag_type;
  }else{
    return 0;
  }
}
int drop_hammer(Map *map, Player *player, int *numCellsToUpdate, void **cellsToUpdate){
  int x,y;

  if(player->hammer != 0){
    fprintf( stderr, "Drop Hammer\n" );
    if(player->hammer == 1){
      		player->hammer = 0;
      		map->hammer_1->p.x = 2;
       		map->hammer_1->p.y = 90;
      		/*find_free_cell(RED, HOME, &(map->hammer_1->p), map);*/
      		map->cells[map->hammer_1->p.x+(map->hammer_1->p.y*MAPHEIGHT)].hammer = map->hammer_1;
      		cellsToUpdate[*numCellsToUpdate] = (void *)&map->cells[map->hammer_1->p.x+(map->hammer_1->p.y*MAPHEIGHT)];
      		(*numCellsToUpdate)++;
    }else if(player->hammer == 2){
      player->hammer = 0;
      map->hammer_2->p.x = 197;
      map->hammer_2->p.y = 90;
	/*find_free_cell(GREEN, HOME, &(map->hammer_2->p), map);*/
	map->cells[map->hammer_2->p.x+(map->hammer_2->p.y*MAPHEIGHT)].hammer = map->hammer_2;
      cellsToUpdate[*numCellsToUpdate] = (void *)&map->cells[map->hammer_2->p.x+(map->hammer_2->p.y*MAPHEIGHT)];
      (*numCellsToUpdate)++;
    }
	player->timestamp = (unsigned)time(NULL);
    return player->hammer;
  }else{
    return 0;
  }
}


int check_win_condition(Map *map, int numplayers, int num_red, int num_green, Player **player_array){
  int i;
  int red_at_home = 0;
  int green_at_home = 0;
  int red_jailed = 0;
  int green_jailed = 0;

  for(i=0;i<MAXPLAYERS;i++){
    Player *p = (Player*)player_array[i];
    if (p == NULL) {
      continue;
    }
    int x = p->pos.x;
    int y = p->pos.y;

    if(p->team_color == RED){
      if(map->cells[x+(y*MAPHEIGHT)].t == HOME && map->cells[x+(y*MAPHEIGHT)].c == RED){
        red_at_home++;
      }else if(p->state == 1){
        red_jailed++;	
      }
    }

    if(p->team_color == GREEN){
      if(map->cells[x+(y*MAPHEIGHT)].t == HOME && map->cells[x+(y*MAPHEIGHT)].c == GREEN){
        green_at_home++;
      }else if(p->state == 1){
        green_jailed++;
      }
    }
  }

  int rf_x, rf_y, gf_x, gf_y;
  rf_x = map->flag_red->p.x;
  rf_y = map->flag_red->p.y;
  gf_x = map->flag_green->p.x;
  gf_y = map->flag_green->p.y;

  //Check for Flags winning the game
  if(red_at_home == num_red && num_red != 0){
    if(map->cells[rf_x+(rf_y*MAPHEIGHT)].t == HOME && map->cells[rf_x+(rf_y*MAPHEIGHT)].c == RED && map->cells[gf_x+(gf_y*MAPHEIGHT)].t == HOME && map->cells[gf_x+(gf_y*MAPHEIGHT)].c == RED){
      //red wins
     fprintf( stderr, "RED TEAM WINS\n" );
     return 0;
    }
  }else if(green_at_home == num_green && num_green != 0){
    if(map->cells[rf_x+(rf_y*MAPHEIGHT)].t == HOME && map->cells[rf_x+(rf_y*MAPHEIGHT)].c == GREEN && map->cells[gf_x+(gf_y*MAPHEIGHT)].t == HOME && map->cells[gf_x+(gf_y*MAPHEIGHT)].c == GREEN){
      //green wins
      fprintf( stderr, "GREEN TEAM WINS\n" );
      return 1;
    }
  }

  //Check for jailing winning the game
  if(red_jailed == num_red && num_red != 0){
    fprintf( stderr, "GREEN TEAM WINS\n" );
    return 1;
  }else if(green_jailed == num_green && num_green != 0){
    fprintf( stderr, "RED TEAM WINS\n" );
    return 0;
  }

  return -1;
}

int valid_move(Map *map, Player *player, int x, int y, int *numCellsToUpdate, void **cellsToUpdate, Player **players, int numplayers, int num_red_players, int num_green_players){
	Cell c;
	c = map->cells[(x + player->pos.x)+((player->pos.y + y)*MAPHEIGHT)];
	if (c.t == WALL){
		if (player->hammer && c.breakable == 0){
			map->cells[(x + player->pos.x)+((player->pos.y + y)*MAPHEIGHT)].t = FLOOR;//convert the destination to floor
			drop_hammer(map, player, numCellsToUpdate, cellsToUpdate);
			return 1;
		}
		return 0;
	}else if (c.player != NULL){
		if (c.player->team_color != player->team_color){
			if (player->team_color == c.c){
 				if(c.player->flag >= 1){
				  drop_flag(map, c.player, numCellsToUpdate, cellsToUpdate);
				}
				if (c.player->hammer > 0){
				  drop_hammer(map, c.player, numCellsToUpdate, cellsToUpdate);
				}
				find_free_cell(c.c, JAIL, &c.player->pos, map);
				map->cells[(c.player->pos.x)+((c.player->pos.y)*MAPHEIGHT)].player = c.player;
				c.player->timestamp = (unsigned)time(NULL);
				c.player->state = 1;
				c.player == NULL;
				cellsToUpdate[*numCellsToUpdate] = (void *)&map->cells[(c.player->pos.x)+((c.player->pos.y)*MAPHEIGHT)];
				(*numCellsToUpdate)++;
				int winner = check_win_condition(map, numplayers, num_red_players, num_green_players, players);
				if(winner == 0){
                                  return 2;
				}else if(winner == 1){
				  return 3;
 				}
				return 1;
			}else{
                                if(player->flag >= 1){
                                  drop_flag(map, player, numCellsToUpdate, cellsToUpdate);
                                }
				if(player->hammer > 0){
				  drop_hammer(map, c.player, numCellsToUpdate, cellsToUpdate);
				}
				find_free_cell(c.c, JAIL, &player->pos, map);
				player->timestamp = (unsigned)time(NULL);
                                map->cells[(player->pos.x)+((player->pos.y)*MAPHEIGHT)].player = player;
				cellsToUpdate[*numCellsToUpdate] = (void *)&map->cells[(player->pos.x)+((player->pos.y)*MAPHEIGHT)];
				player->state = 1;
      				(*numCellsToUpdate)++;
				int winner = check_win_condition(map, numplayers, num_red_players, num_green_players, players);
				if(winner == 0){
                                  return 2;
                                }else if(winner == 1){
                                  return 3;
                                }
				return 0;
			}
		}
		return 0;
	}else if (player->state ==1 && c.t != JAIL){
		return 0;
	}else if (player->state == 0 && c.c != player->team_color && c.t == JAIL){
		int i;
		for (i = 0; i < numplayers;i++){
                        if (players[i]->team_color == player->team_color && players[i]->state == 1){
                                Player *p = (Player*)players[i];
				p->state = 0;
				p->timestamp = (unsigned)time(NULL);
				fprintf( stderr, "Player freed\n" );
                        }
                }
		return 1;
	}else{
		return 1;
	}
	return 1;
}	
char* dump_map(Map *map){ 
  int j, i;
  Cell c;
  for (j = 0; j < MAPHEIGHT; j++){
        for (i = 0; i < MAPWIDTH; i++){
                c = map->cells[i+(j*MAPHEIGHT)];
			/*if(c.hammer != NULL){
                          if(i < 100 && j < 200){
                            map->data_ascii[i+(j*MAPHEIGHT)] = 's';
                          }else{
                            map->data_ascii[i+(j*MAPHEIGHT)] = 'S';
                          }
                        }else*/ if (c.t == FLOOR){
				map->data_ascii[i+(j*MAPHEIGHT)] = ' ';
			}else if (c.t == WALL){
				map->data_ascii[i+(j*MAPHEIGHT)] = '#';
			}else if (c.t == HOME && c.c == RED ){
                        	map->data_ascii[i+(j*MAPHEIGHT)] = 'h';
                	}else if (c.t == HOME && c.c == GREEN ){
                                map->data_ascii[i+(j*MAPHEIGHT)] = 'H';
                        }else if (c.t == JAIL && c.c == RED ){
                                map->data_ascii[i+(j*MAPHEIGHT)] = 'j';
                        }else if (c.t == JAIL && c.c == GREEN ){
                                map->data_ascii[i+(j*MAPHEIGHT)] = 'J';
                        }
	}
  }
  return (char*)&map->data_ascii;
}
int map_loop_team(Color c, Cell_Type t, Map *map){
  int cells=0;  
  int i,j = 0;
  
  for(i=0;i<map->h;i++){
    for(j=0;j<map->w;j++){
      Cell cur_cell = map->cells[j+(i*MAPHEIGHT)];
      if(cur_cell.t == t && cur_cell.c == c){
        cells++;  
      }   
    }
  } 
  
  return cells;
}

int num_home(Color c, Map *map){
  int cells=0;
  Cell_Type t = HOME;

  cells = map_loop_team(c, t, map); 

  return cells;
}

int num_jail(Color c, Map *map){
  int cells=0;
  Cell_Type t = JAIL;

  cells = map_loop_team(c, t, map);

  return cells;
}
int num_floor(Map *map){
  return map->num_floor_cells;
}

int num_wall(Map *map){
  return map->num_wall_cells;
}

int dim(Map *map, Pos *d){
  d->x = map->w;
  d->y = map->h;
  return 1;
}

int cinfo(Map *map, Cell *cell, int x, int y){
  if(x > map->w-1 || y > map->h-1){
    return -1;
  }else{
    *cell = map->cells[x+(y*MAPHEIGHT)];
  }

  return 1;
}
