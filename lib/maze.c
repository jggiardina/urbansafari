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

int num_home(Color c){
  int cells=0;

  if(c == RED){
     
  }else if(c == GREEN){

  }

  return cells;
}

int num_jail(Color c){
  int cells=0;

  if(c == RED){

  }else if(c == GREEN){

  }

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
