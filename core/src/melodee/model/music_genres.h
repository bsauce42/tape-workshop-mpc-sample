// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop: shared genre vocabulary and original musical profiles.
#pragma once
#include <algorithm>
#include <cstdint>
namespace melodee::music {
inline constexpr unsigned genre_count=28;
inline const char* genre_name(unsigned n){static const char* names[]={"Hip Hop","Trap","Boom Bap","UK Drill","Lo-Fi","R&B","Neo Soul","House","Deep House","Techno","Drum & Bass","Jungle","Dubstep","Afrobeats","Dancehall","Reggaeton","Latin","Funk","Disco","Pop","Rock","Jazz","Ambient","Country","Synthwave","Gospel","Trance","Future Bass"};return names[n%genre_count];}
// Melody-generator IDs: zero preserves the original behavior; 1..28 match
// the drum styles in order. These settings are not project-format fields.
inline const char* harmony_genre_name(unsigned id){return id?genre_name((id-1)%genre_count):"Original";}
struct Profile {uint8_t chords,pluck,melody,path,rotation,hop;int8_t velocity;float colour;};
inline Profile profile(unsigned id){
 static constexpr Profile styles[]={
 {2,9,18,0,0,3,-2,.20f},{18,6,8,3,1,5,3,-.20f},{11,4,19,0,2,3,-4,.25f},{17,8,13,3,1,7,2,-.15f},
 {1,18,20,5,0,2,-14,.45f},{14,9,11,2,2,3,-7,.55f},{9,11,19,5,1,2,-9,.65f},{10,4,4,1,0,5,2,.10f},
 {10,9,18,0,2,3,-7,.35f},{3,5,9,6,1,2,5,-.25f},{2,5,13,2,2,5,3,.10f},{11,6,9,4,1,7,1,.25f},
 {2,8,18,3,0,7,5,-.15f},{12,12,9,1,1,3,0,.15f},{12,9,12,1,2,5,0,.10f},{9,12,9,3,0,3,2,-.05f},
 {12,21,12,4,1,5,0,.35f},{9,5,11,6,2,3,2,.35f},{10,4,4,1,1,5,2,.20f},{2,4,18,2,0,3,0,-.10f},
 {3,4,19,1,0,7,5,-.35f},{11,6,6,6,2,2,-9,.85f},{1,1,1,5,1,2,-20,.50f},{2,4,21,1,0,5,-2,-.10f},
 {2,5,4,0,1,5,1,.15f},{11,9,19,4,2,3,-3,.70f},{3,5,5,2,0,7,4,-.05f},{18,13,9,3,1,5,1,.25f}};
 return id&&id<=genre_count?styles[id-1]:Profile{0,0,0,0,0,3,0,0};
}
inline unsigned swing(unsigned id){switch(id){case 1:return 55;case 3:return 58;case 5:return 60;case 6:return 55;case 7:return 58;case 9:return 55;case 12:return 55;case 18:return 56;case 22:return 60;case 24:return 52;case 26:return 55;default:return 50;}}
inline double quality_bonus(unsigned genre,unsigned quality,unsigned category){if(!genre)return quality==1||quality==5||quality==9?.25:0;auto p=profile(genre);return category==1?-p.colour*.3:category==2?p.colour*.65:p.colour;}
}
