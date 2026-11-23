// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/shell/harmony_tool.h"
#include <cassert>
#include <iostream>
using namespace melodee;
int main() {
    // Automatic WAV detection starts only once a WAV pad is confirmed, not
    // while browsing source types, MIDI pads, or later output settings.
    HarmonyTool keyFlow;keyFlow.mode=2;harmony_step_begin(keyFlow,0);
    assert(!harmony_detect_on_confirm(keyFlow));
    harmony_step_forward(keyFlow);assert(!harmony_detect_on_confirm(keyFlow));
    harmony_step_forward(keyFlow);assert(harmony_detect_on_confirm(keyFlow));
    keyFlow.mode=1;assert(!harmony_detect_on_confirm(keyFlow));keyFlow.mode=2;
    harmony_step_forward(keyFlow);assert(!harmony_detect_on_confirm(keyFlow));

    for(unsigned mode=0;mode<3;++mode)for(unsigned pad=0;pad<2;++pad){
        HarmonyTool h;h.mode=mode;h.midi_read=pad;harmony_step_begin(h,0);
        auto fields=harmony_step_rows(h);assert(fields.at[0]==13);
        unsigned visited=0;
        while(h.step==0){assert(h.row==fields.at[visited++]);harmony_step_forward(h);assert(visited<=fields.count);}
        assert(visited==fields.count&&h.step==1&&h.row==17);
        harmony_step_forward(h);assert(h.row==6&&h.step==1);harmony_step_forward(h);assert(h.row==33&&h.step==1);harmony_step_forward(h);assert(h.row==30&&h.step==1);harmony_step_forward(h);assert(h.step==2&&h.row==10);
        harmony_step_forward(h);assert(h.step==3&&h.row==31);
        harmony_step_forward(h);assert(h.step==4&&h.row==32);
        harmony_step_back(h);assert(h.step==3&&h.row==31);
        harmony_step_back(h);harmony_step_back(h);harmony_step_back(h);
        assert(h.step==0&&h.row==13);
        harmony_step_forward(h);auto second=h.row;harmony_step_back(h);assert(h.row==13&&second!=13);
        for(unsigned output=0;output<5;++output){h.parts.output=output;if(output==1){harmony_step_begin(h,1);assert(harmony_step_rows(h).count==5);harmony_step_forward(h);assert(h.row==6&&h.step==1);harmony_step_forward(h);assert(h.row==33&&h.step==1);harmony_step_forward(h);assert(h.row==30&&h.step==1);harmony_step_forward(h);assert(h.row==19);harmony_step_forward(h);assert(h.step==2);}
        auto options=harmony_rows(h);assert(options.count&&options.count<=34);for(unsigned n=0;n<options.count;++n)assert((options.at[n]<=30||options.at[n]==33));}
    }
    std::cout<<"Guided source branches, all five steps, forward/back, selected fields and bounded options passed.\n";
}
