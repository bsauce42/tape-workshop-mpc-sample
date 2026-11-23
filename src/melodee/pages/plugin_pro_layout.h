// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Tape Workshop contributors
#pragma once
#include <algorithm>
#include <string_view>
namespace melodee {
enum class ProInstrument { None,Afterglow,Grand,Choir,Loom,Stillwater,Dex,Sloop,Layer,Aphex,Braids,Dexed,Hera,Plaits,Break,Gate,Rack,Vocal };
enum class ProPlot { Keys,Room,Choir,Sources,Waterfall,Operators,Algorithm,Wavetable,Phase,Filter,Envelope,Offsets,Lfo,Delay,Reverb,Mixer,Drums,Gate,Compression,Pitch,Model };
enum class ProMaterial { Walnut,Ebony,Brushed,Cream,Graphite,BlueSteel,Burgundy,Slate };
struct ProPlan { const char* title;ProPlot plot; };
struct ProStyle { ProInstrument instrument;ProMaterial material;unsigned accent,background;bool dark_text=false; };
inline std::string_view pro_name(std::string_view name){if(name.size()>5&&name.substr(name.size()-5)==" (TW)")name.remove_suffix(5);return name;}
inline ProStyle pro_style(std::string_view original) {
 const auto n=pro_name(original);
 if(n=="Afterglow Piano")return{ProInstrument::Afterglow,ProMaterial::Graphite,0xff57ff00,0xff061205};
 if(n=="Room Grand")return{ProInstrument::Grand,ProMaterial::Ebony,0xffe2d4a5,0xff171817};
 if(n=="Velvet Choir")return{ProInstrument::Choir,ProMaterial::Walnut,0xffdfaa99,0xff36221d};
 if(n=="Astral Loom")return{ProInstrument::Loom,ProMaterial::Graphite,0xffb99aff,0xff1a1827};
 if(n=="Stillwater")return{ProInstrument::Stillwater,ProMaterial::Slate,0xff6fcacc,0xff193338};
 if(n=="Dex FM")return{ProInstrument::Dex,ProMaterial::Graphite,0xff96e98d,0xff202320};
 if(n=="Sloop FM6")return{ProInstrument::Sloop,ProMaterial::BlueSteel,0xff82d6ff,0xff253e54};
 if(n=="Layer House")return{ProInstrument::Layer,ProMaterial::Slate,0xff72e1db,0xff24343e};
 if(n=="Aphex Move"||n=="Aphex")return{ProInstrument::Aphex,ProMaterial::Burgundy,0xffffb2d0,0xff402634};
 if(n=="Braids Move"||n=="Braids")return{ProInstrument::Braids,ProMaterial::Cream,0xff41697b,0xffddd9c7,true};
 if(n=="Dexed Move"||n=="Dexed")return{ProInstrument::Dexed,ProMaterial::Brushed,0xffa8cc77,0xff42413d};
 if(n=="Hera Move"||n=="Hera")return{ProInstrument::Hera,ProMaterial::Walnut,0xff93bcdc,0xff343b44};
 if(n=="Plaits Move"||n=="Plaits")return{ProInstrument::Plaits,ProMaterial::Cream,0xffb04f2e,0xffe3dfd0,true};
 if(n=="Break 26")return{ProInstrument::Break,ProMaterial::Brushed,0xffffb260,0xff3c3830};
 if(n=="Gate Keeper")return{ProInstrument::Gate,ProMaterial::Graphite,0xff8fe4ae,0xff202622};
 if(n=="Rack 76")return{ProInstrument::Rack,ProMaterial::Brushed,0xffdfc387,0xff282b2e};
 if(n=="Vocal Tune")return{ProInstrument::Vocal,ProMaterial::Slate,0xffb3dcdf,0xff243b44};
 return{ProInstrument::None,ProMaterial::Graphite,0xffc49aff,0xff16151b};
}
inline ProPlan pro_plan(ProInstrument i,unsigned row) {
 using I=ProInstrument;using P=ProPlot;
 switch(i) {
 case I::Afterglow: {
  constexpr ProPlan a[]={{"PIANO / HAMMER",P::Keys},{"PROGRESSION",P::Keys},{"DECAY / SHAPE",P::Envelope},{"SCALE / ROOT",P::Keys},{"ARPEGGIATOR",P::Delay},{"VOICING",P::Keys},{"SONG KEY",P::Keys},{"FEEL / RESPONSE",P::Mixer},{"NOTE INPUT",P::Keys}};return a[std::min<unsigned>(row,8)];}
 case I::Grand: {constexpr ProPlan a[]={{"GRAND PIANO",P::Keys},{"HAMMER / TOUCH",P::Mixer},{"ROOM / REFLECTIONS",P::Room},{"INSTRUMENT / NOISE",P::Sources},{"TREMOLO / DRIVE",P::Lfo}};return a[std::min<unsigned>(row,4)];}
 case I::Choir: {constexpr ProPlan a[]={{"CHOIR / VOWELS",P::Choir},{"LOW / HIGH BLEND",P::Choir},{"LOW VOICE",P::Filter},{"HIGH VOICE",P::Filter},{"ENSEMBLE WIDTH",P::Phase},{"PHRASE / TEMPO",P::Delay},{"CHOIR / HUMANIZE",P::Choir},{"VOICE / BREATH",P::Sources}};return a[std::min<unsigned>(row,7)];}
 case I::Loom: {
  if(row==0)return{"LIVE STEREO AUDIO",P::Sources};if(row<=8)return{row%2?"LAYER SOURCE / TUNE":"LAYER AMP ENVELOPE",row%2?P::Sources:P::Envelope};
  constexpr ProPlan a[]={{"AMP / VELOCITY",P::Envelope},{"VIBRATO / UNISON",P::Lfo},{"DELAY / AMBIENCE",P::Reverb},{"FEEDBACK / TAIL",P::Delay},{"ARPEGGIATOR",P::Delay},{"ARP / FILTER ENV",P::Envelope},{"THREE BAND TONE",P::Filter},{"FILTER / EFFECTS",P::Waterfall}};return a[std::min<unsigned>(row-9,7)];}
 case I::Stillwater: {constexpr ProPlan a[]={{"VOICE / BLEND",P::Phase},{"AMP ENVELOPE",P::Envelope},{"DRIFT / STEREO",P::Phase},{"BELLS",P::Sources},{"ECHO",P::Delay},{"REVERB",P::Reverb},{"SUB / FIFTH / OUTPUT",P::Mixer}};return a[std::min<unsigned>(row,6)];}
 case I::Dex:case I::Sloop:
  if(row==0)return{"PATCH / FM VOICE",P::Operators};if(row==1)return{"ALGORITHM / MOD TIME",P::Algorithm};
  if(row==2)return{"STEREO / VELOCITY",P::Mixer};if(row==3)return{"FM MODULATION",P::Lfo};
  if(row<16)return{row%2?"OPERATOR TIME OFFSETS":"OPERATOR TUNING",row%2?P::Offsets:P::Operators};
  return{row%2?"OPERATOR KEY SCALING":"OPERATOR LEVEL / TIME",row%2?P::Filter:P::Offsets};
 case I::Layer: {constexpr ProPlan a[]={{"OSCILLATORS A / B",P::Wavetable},{"LAYER A ENVELOPE",P::Envelope},{"LAYER B ENVELOPE",P::Envelope},{"ATTACK LAYER",P::Sources},{"SPREAD / MOTION",P::Phase},{"ECHO / SPACE",P::Reverb},{"WAVETABLE MORPH",P::Wavetable},{"WARP / RING MOD",P::Wavetable},{"FILTER / SUB",P::Filter},{"CHORUS",P::Phase},{"FX / ROUTING",P::Mixer},{"VELOCITY / OUTPUT",P::Mixer},{"WAVETABLE VIEW",P::Wavetable},{"ENVELOPE",P::Envelope},{"WARP VIEW",P::Wavetable},{"RELEASE / TONE",P::Envelope}};return a[std::min<unsigned>(row,15)];}
 case I::Aphex:return row?ProPlan{"MASTER OUTPUT",P::Mixer}:ProPlan{"PRESET / LOW PASS",P::Filter};
 case I::Braids: {constexpr ProPlan a[]={{"MACRO OSCILLATOR",P::Model},{"AMP ENVELOPE",P::Envelope},{"RELEASE / FILTER",P::Filter},{"ENVELOPE / FM",P::Lfo},{"OCTAVE / ATTACK",P::Envelope},{"SUSTAIN / RELEASE",P::Envelope}};return a[std::min<unsigned>(row,5)];}
 case I::Dexed: {constexpr ProPlan a[]={{"FM PRESET",P::Algorithm},{"ALGORITHM / FEEDBACK",P::Algorithm},{"PITCH / LFO",P::Lfo},{"MODULATION DEPTH",P::Lfo},{"LFO / OPERATOR 1",P::Operators},{"OPERATORS 2 / 3 / 4",P::Operators},{"OPERATORS 5 / 6",P::Operators}};return a[std::min<unsigned>(row,6)];}
 case I::Hera: {constexpr ProPlan a[]={{"VOICE / LOW PASS",P::Filter},{"FILTER ENVELOPE",P::Envelope},{"AMP ENVELOPE",P::Envelope},{"LFO RATE",P::Lfo}};return a[std::min<unsigned>(row,3)];}
 case I::Plaits: {constexpr ProPlan a[]={{"SYNTHESIS ENGINE",P::Model},{"MORPH / DECAY",P::Waterfall},{"LPG / FM / MIX",P::Filter},{"TIMBRE / MORPH MOD",P::Envelope},{"LEGATO / RESPONSE",P::Keys}};return a[std::min<unsigned>(row,4)];}
 case I::Break:return row?ProPlan{"DRIVE / MIX / LEVEL",P::Mixer}:ProPlan{"RATE / BITS / TONE",P::Drums};
 case I::Gate: {constexpr ProPlan a[]={{"GATE TRANSFER",P::Gate},{"ATTACK / HOLD / RELEASE",P::Envelope},{"STYLE / KNEE",P::Gate},{"SIDECHAIN FILTER",P::Filter},{"LINK / DETECTOR",P::Phase},{"WET / DRY / TRIM",P::Mixer},{"PAN / HYSTERESIS",P::Gate}};return a[std::min<unsigned>(row,6)];}
 case I::Rack: {constexpr ProPlan a[]={{"INPUT / RATIO / OUTPUT",P::Compression},{"FET TIMING",P::Compression},{"CIRCUIT / METER",P::Compression},{"PARALLEL BLEND",P::Mixer}};return a[std::min<unsigned>(row,3)];}
 case I::Vocal: {constexpr ProPlan a[]={{"KEY / SCALE / RETUNE",P::Pitch},{"CORRECTION / HUMANIZE",P::Pitch},{"VOICE / TRACKING",P::Pitch}};return a[std::min<unsigned>(row,2)];}
 default:return{"OUTPUT",P::Waterfall};
 }
}
}
