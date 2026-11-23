#include "melodee/model/bar_selection.h"
#include <cassert>
int main(){
 using melodee::bar_knob_index;
 assert(bar_knob_index(0,8*384,384)==0);
 assert(bar_knob_index(126,8*384,384)==7);
 assert(bar_knob_index(127,8*384,384)==7);
 assert(bar_knob_index(126,24*384+100,384)==24);
 assert(bar_knob_index(127,384,384)==0);
 assert(bar_knob_index(-1,8*384,384)==0);
 assert(bar_knob_index(200,8*384,384)==7);
 assert(bar_knob_index(127,0,384)==0);
 for(unsigned count=1;count<=128;++count){bool seen[128]={};for(int v=0;v<128;++v){auto i=bar_knob_index(v,count*384,384);assert(i<count);seen[i]=true;}for(unsigned n=0;n<count;++n)assert(seen[n]);}
}
