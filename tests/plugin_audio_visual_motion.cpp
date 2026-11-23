// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/plugin/audio_visual_motion.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace melodee;PluginAudioMotion m;
 assert(m.update(0,1,.5f,true)==0);
 const float first=m.update(30,2,.5f,true);assert(first>0);
 assert(m.update(60,3,0,true)==first); // silent audio, even if transport is playing
 assert(m.update(90,4,.5f,false)==first); // bypass
 const float resumed=m.update(120,5,.5f,true);assert(resumed>first);
 assert(m.update(250,5,.5f,true)==resumed); // stale held waveform cannot keep reels turning
 assert(m.update(280,6,0,true)==resumed); // stop holds the current hub position
 assert(m.update(310,7,.2f,true)>resumed); // resumes smoothly without a wall-clock jump
 std::cout<<"PASS: audio-driven reel motion, silence, bypass, stale data, smooth resume\n";
}
