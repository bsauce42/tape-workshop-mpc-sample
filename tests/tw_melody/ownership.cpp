// SPDX-License-Identifier: AGPL-3.0-only
// Exercise voice ownership without invoking the render callback or producing audio.
#define TW_ORCHESTRA 1
#include "../../plugins/tape-workshop-violin/strings.cpp"
#include <cassert>
int main(){
    auto* effect=VSTPluginMain(nullptr);assert(effect);auto& synth=self(effect);
    vst2::MidiEvent manual{};manual.data[0]=char(0x90);manual.data[1]=60;manual.data[2]=100;
    synth.midi(manual);synth.melodyNote(60,100);
    auto held=[&](bool generated){unsigned count=0;for(const auto& v:synth.voices)count+=v.active&&v.held&&v.generated==generated;return count;};
    assert(held(false)==1&&held(true)==1);
    manual.data[0]=char(0x80);synth.midi(manual);assert(!held(false)&&held(true)==1);
    manual.data[0]=char(0x90);synth.midi(manual);synth.stopMelody();assert(held(false)==1&&!held(true));
    synth.melodyNote(64,100);manual.data[0]=char(0xb0);manual.data[1]=120;manual.data[2]=0;synth.midi(manual);
    assert(!held(true)&&!held(false)&&synth.melodyBlocked);
    effect->dispatcher(effect,vst2::op::close,0,0,nullptr,0);
    std::puts("Generated note releases preserve manual notes; same-pitch note-offs are isolated; panic blocks regeneration. No audio rendered.");
}
