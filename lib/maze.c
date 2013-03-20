#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

#include "misc.h"
#include "maze.h"

int load_map(char* map_file, Map *map){
  int rc=1;
  Cell ctest;
  map->data_ascii = map_file;
  int i, j;
  for (j = 0; j < 200; j++){
  	for (i = 0; i < 200; i++){
		Cell c;
             if (i > 99) {
                                c.c = GREEN;
                        }else{
                                c.c = RED;
                        }
                        switch(map_file[i+(j*200)]){
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
                                        return -1;
                        }
                        c.p.x = i;
                        c.p.y = j;
                        map->cells[i+(j*200)] = c;
			if (i+j == (198)){
				fprintf(stderr, "Last cell type = %d, Team = %d, x = %d, y = %d\n", c.t, c.c, c.p.x, c.p.y);
			}
                }
  }
  fprintf(stderr, "Read in %d rows and %d columns\n", j, i);
  return rc;
}

char* dump_map(Map map){
 /* 
  int j, i;
  Cell c;
  for (j = 0; j < 200; j++){
        for (i = 0; i < 200; i++){
                c = map.cells[i+j];
		if (map.cells[i+j].t == FLOOR){
		}
                        switch(map.cells[i+j].t){
                                case JAIL:
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
                                        return -1;
                        }
                        c.p.x = i;
                        c.p.y = j;
                        map->cells[i+j] = c;
                }
  }*/
  return map.data_ascii;
}

int map_loop_team(Color c, Cell_Type t, Map map){
  int cells=0;  
  int i,j = 0;
  
  for(i=0;i<map.h;i++){
    for(j=0;j<map.w;j++){
      Cell cur_cell = map.cells[i,j];
      if(cur_cell.t == t && cur_cell.c == c){
        cells++;  
      }   
    }
  } 
  
  return cells;
}

int num_home(Color c, Map map){
  int cells=0;
  Cell_Type t = HOME;

  cells = map_loop_team(c, t, map); 

  return cells;
}

int num_jail(Color c, Map map){
  int cells=0;
  Cell_Type t = JAIL;

  cells = map_loop_team(c, t, map);

  return cells;
}

int num_floor(Map map){
  return map.num_floor_cells;
}

int num_wall(Map map){
  return map.num_wall_cells;
}

Pos* dim(Map map){
  Pos d;
  d.x = 10;//map.w;
  d.y = 20;//map.h;
  return &d;
}

int cinfo(){
  NYI;assert(0);
}
