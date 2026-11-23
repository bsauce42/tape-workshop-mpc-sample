// SPDX-License-Identifier: AGPL-3.0-only
// Real Second Spin DSP + production idle gate: zero must be reversible without a new note.
#include "melodee/plugin/idle_gate.h"
extern "C" {
#include "engine.h"
}
#include <array>
#include <cassert>
#include <cstdio>
#include <algorithm>
int main(){
 const auto* api=mpc_engine();void* effect=api->create(nullptr);assert(effect);
 melodee::IdleGate gate;gate.reset(false,44100);
 std::array<int16_t,256> in{},out{};std::array<float,128> l{},r{};
 auto block=[&](){
  if(gate.asleep(false,false,l.data(),r.data(),128)){out.fill(0);return 0;}
  api->process(effect,in.data(),out.data(),128);
  int peak=0;for(unsigned n=0;n<128;++n){l[n]=out[n*2]/32768.f;r[n]=out[n*2+1]/32768.f;peak=std::max({peak,std::abs(int(out[n*2])),std::abs(int(out[n*2+1]))});}
  gate.heard(l.data(),r.data(),128);l.fill(0);r.fill(0);return peak;
 };
 int peak=0;for(int n=0;n<100;++n)peak=std::max(peak,block());assert(peak>0);
 api->set_param(effect,"crackle","0");
 for(int n=0;n<700;++n)block();assert(gate.asleep(false,false,l.data(),r.data(),128));
 api->set_param(effect,"crackle","50");assert(block()==0); // reproduces old missing-wake bug
 gate.request_wake();peak=0;for(int n=0;n<100;++n)peak=std::max(peak,block());assert(peak>10);
 for(int repeat=0;repeat<3;++repeat){
  api->set_param(effect,"crackle","0");gate.request_wake();for(int n=0;n<700;++n)block();
  assert(gate.asleep(false,false,l.data(),r.data(),128));
  api->set_param(effect,"crackle","35");gate.request_wake();peak=0;for(int n=0;n<100;++n)peak=std::max(peak,block());assert(peak>10);
 }
 api->destroy(effect);std::puts("PASS real DSP: Crackle zero sleeps; returning above zero wakes repeatedly without notes; silent idle remains efficient");
}
