#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

//#include "misc.h"
#include "maze.h"

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
  map->w = MAPWIDTH;
  map->h = MAPHEIGHT;
  fprintf(stderr, "Read in %d rows and %d columns\n", j, i);
  return rc;
}


int valid_move(Map *map, Player *player, int x, int y){
	//x, y are the destination coords. we get the current pos of player from *player->x/y
	Cell c;
	Player p;
	//previousc = map->cells[player->x +(player->y*MAPHEIGHT)];
	c = map->cells[(x + player->pos.x)+((player->pos.y + y)*MAPHEIGHT)];
	if (c.t == WALL){

		/*if (player->hammer){
			map->cells[x+(Y*MAPHEIGHT)].t = FLOOR;//convert the destination to floor
			player->pos.x = x;
			player->pos.y = y;
			//need to decrease hammer count, not sure how to do so yet
			//trigger break wall event
			return 1;
		
		return -1;
		}else{*/
		//INVALID MOVE
		return 0;
	}/*else{
	//check if collides with player
	for (int i = 0; i < numplayers; i++){
		p = players[i];
		if (p.x == x && p.y == y){//colliding players
			if (p.c != player->c){
				if (player->c != c.c && player->state != 0){//moving player will be jailed
					//NYI- send player to jail function
					player->state == 1;
					return 1;
				}else if (p.c != c.c && p.state != 0){//stationary player will be jailed
					//NYI- send stationary player to jail function
					p.state == 1;
					player->x = x;
					player->y = y;
					//check flag
					return 1;
				}
				
			}else{
				//INVALID MOVE
				return 0;
			}
			
		}	
	}
	if (c.t == JAIL && player->c != c.c && previousc.t != JAIL){//Jailbreak
		for (int i = 0; i < numplayers; i++){
                	p = players[i];
			if (p.state == JAILED && p.c == player->c){
				p.state = FREE;
			}
		}
		
	}
	//else check if jail, if foreign jail then free all players
	}*/
	return 1;
}	
//wall collision
//whether 5 away from flag
//player collision
//jail collision


/*TO ADD:
UPDATE ALL PLAYERS EVENT
UPDATE ALL FLAGS EVENT
*/ 
char* dump_map(Map *map){ 
  int j, i;
  Cell c;
  for (j = 0; j < MAPHEIGHT; j++){
        for (i = 0; i < MAPWIDTH; i++){
                c = map->cells[i+(j*MAPHEIGHT)];
			if (c.t == FLOOR){
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
