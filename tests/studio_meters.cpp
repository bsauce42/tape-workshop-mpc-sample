// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/web/studio_meters.h"
#include <cassert>
#include <limits>
#include <iostream>
struct Source{float meter(unsigned t)const{return t==0?.5f:0.f;}float out_level(unsigned s)const{return s==0?1.f:.1f;}int position_of(unsigned)const{return 12;}unsigned pattern_of(unsigned)const{return 0;}unsigned ticks()const{return 48;}unsigned song_bar()const{return 0;}bool playing()const{return true;}bool song_playing()const{return false;}};
int main(){using namespace melodee::web;assert(studio_peak_db10(0)==-960);assert(studio_peak_db10(1)==0);assert(studio_peak_db10(.5f)==-60);assert(studio_peak_db10(.1f)==-200);assert(studio_peak_db10(std::numeric_limits<float>::quiet_NaN())==-960);assert(studio_peak_db10(-1)==-960);assert(studio_peak_db10(2)==60);auto json=studio_meter_json(Source{});assert(json.find("[-60,-960,")!=std::string::npos);assert(json.find("\"left\":0,\"right\":-200")!=std::string::npos);std::cout<<"PASS actual peak-to-dBFS conversion and independent stereo output\n";}
