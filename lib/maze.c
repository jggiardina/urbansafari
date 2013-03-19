#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

#include "misc.h"
#include "maze.h"

int load_map(char* map_file, Map map){
  int rc=1;

  return rc;
}

char* dump_map(Map map){
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

char* dim(){
  NYI;assert(0);
  //return (char*)map.h + 'x' + (char*)map.w;
}

int cinfo(){
  NYI;assert(0);
}
