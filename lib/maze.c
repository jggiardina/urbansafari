#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>

#include "misc.h"
#include "maze.h"

Map map;

int load_map(char* map_file){
  int rc=1;

  return rc;
}

int dump_map(Map* m){
  int rc=1;

  return rc;
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

int num_floor(){
  return map.num_floor_cells;
}

int num_wall(){
  return map.num_wall_cells;
}

char* dim(){
  NYI;assert(0);
  //return (char*)map.h + 'x' + (char*)map.w;
}

int cinfo(){
  NYI;assert(0);
}