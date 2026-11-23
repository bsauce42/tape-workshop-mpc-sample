// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/pages/plugin_pro_layout.h"
#include <cassert>
#include <cstring>
#include <iostream>
int main(){using namespace melodee;
for(auto name:{"Afterglow Piano","Room Grand","Velvet Choir","Astral Loom","Stillwater","Dex FM","Sloop FM6","Layer House","Aphex Move","Braids Move","Dexed Move","Hera Move","Plaits Move","Break 26","Gate Keeper","Rack 76","Vocal Tune"}){
auto style=pro_style(name);assert(style.instrument!=ProInstrument::None);for(unsigned row=0;row<40;++row)assert(std::strlen(pro_plan(style.instrument,row).title)>0);}
for(auto name:{"Sub Foundry","Contour Comp","Prism EQ","Second Spin","Third Party"})assert(pro_style(name).instrument==ProInstrument::None);
assert(pro_style("Dex FM (TW)").instrument==ProInstrument::Dex);
assert(pro_plan(ProInstrument::Grand,2).plot==ProPlot::Room);
assert(pro_plan(ProInstrument::Layer,0).plot==ProPlot::Wavetable);
assert(pro_plan(ProInstrument::Afterglow,2).plot==ProPlot::Envelope);
std::cout<<"Distinct profiles, page mapping, aliases and protected exclusions passed\n";
}
