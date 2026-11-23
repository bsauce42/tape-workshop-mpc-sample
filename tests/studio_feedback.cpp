// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/web/control_feed.h"
#include <cassert>
#include <iostream>
int main(){melodee::web::ControlFeed feed;assert(feed.notes_json().find("\"events\":[]")!=std::string::npos);for(unsigned i=0;i<70;i++)feed.live_note(i%16,36+i%128,100);const auto s=feed.notes_json();assert(s.find("\"sequence\":70")!=std::string::npos);assert(s.find("[7,6,42,")!=std::string::npos);assert(s.find("[70,5,105,")!=std::string::npos);assert(s.find("[1,0,36,")==std::string::npos);feed.live_note(16,36,100);feed.live_note(0,36,0);assert(feed.notes_json().find("\"sequence\":70")!=std::string::npos);std::cout<<"PASS fixed live-note ring retains newest 64 mapped input events, bounds invalid input, and seeds without replay\n";}
