// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/plugin/meter_page.h"
#include <cassert>
#include <iostream>
int main() {
    using namespace melodee;
    PluginPanel panel;
    std::snprintf(panel.name,sizeof panel.name,"Second Spin");
    for(unsigned row=0;row<7;++row) {
        configure_workshop_pages(panel,21,row);
        assert(panel.rows==8 && panel.row==row && !panel.meter_page);
        assert(panel.row*3<21); // existing parameter indices stay unchanged
    }
    configure_workshop_pages(panel,21,7);
    assert(panel.rows==8 && panel.row==7 && panel.meter_page);
    std::vector<int> shown;for(int n=0;n<21;++n)shown.push_back(n);
    configure_workshop_meter_aliases(panel,shown);
    assert(panel.parameter_index(0)==9 && panel.parameter_index(1)==10 && panel.parameter_index(2)==4);
    std::reverse(shown.begin(),shown.end());configure_workshop_meter_aliases(panel,shown);
    assert(shown[panel.parameter_index(0)]==9 && shown[panel.parameter_index(1)]==10 && shown[panel.parameter_index(2)]==4);
    panel.entry=4;
    for(unsigned k=0;k<3;++k) {
        panel.params[k].used=true;panel.params[k].value=int16_t(100+k*200);
        assert(plugin_card(panel,plugin_binding(4,panel.parameter_index(k)))==&panel.params[k]);
    }
    assert(plugin_card(panel,plugin_binding(3,panel.parameter_index(0)))==nullptr);
    assert(plugin_card(panel,plugin_binding(4,20))==nullptr);
    configure_workshop_pages(panel,21,2);assert(panel.parameter_index(0)==6 && panel.parameter_index(2)==8);
    configure_workshop_pages(panel,21,999);
    assert(panel.row==7 && panel.meter_page);
    for(auto name:{"Sub Foundry","Contour Comp","Prism EQ","Third-party Synth"}) {
        std::snprintf(panel.name,sizeof panel.name,"%s",name);
        configure_workshop_pages(panel,15,999);
        assert(panel.rows==5 && panel.row==4 && !panel.meter_page);
    }
    std::snprintf(panel.name,sizeof panel.name,"Dex FM (TW)");
    configure_workshop_pages(panel,84,28);
    assert(panel.rows==29 && panel.row==28 && panel.meter_page);
    std::cout<<"PASS: editable meter bank aliases, reordered skins, existing indices and protected plug-ins unchanged\n";
    // Bloom's first page is IR, with all saved plug-in parameter IDs unchanged.
    panel={};std::strcpy(panel.name,"Bloom Reverb");
    constexpr unsigned banks[]={6,0,1,2,3,4,5,7};
    for(unsigned row=0;row<8;++row){configure_workshop_pages(panel,24,row);assert(panel.rows==8&&!panel.meter_page&&panel.bloom_ir_first);for(unsigned k=0;k<3;++k)assert(panel.parameter_index(k)==banks[row]*3+k);}
}
