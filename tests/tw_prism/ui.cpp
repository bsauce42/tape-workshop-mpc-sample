#include "melodee/plugin/meter_page.h"
#include <cassert>
#include <cstring>
int main(){melodee::PluginPanel p;std::strcpy(p.name,"Prism FM");melodee::configure_workshop_pages(p,180,999);assert(p.prism&&p.rows==64&&p.row==63);bool seen[180]{};for(unsigned row=0;row<64;++row)for(unsigned k=0;k<3;++k){unsigned index=p.parameter_index_at(row,k);assert(index<180);seen[index]=true;}for(bool v:seen)assert(v);assert(p.parameter_index_at(9,2)==33);assert(p.parameter_index_at(62,0)==174);}
