// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/model/harmonizer.h"
#include "melodee/model/smf.h"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <tuple>
using namespace melodee;
int main(){using namespace harmony;std::vector<harmony::Note> notes{{0,96,72,110},{96,48,74,65},{144,24,73,45},{192,192,76,120},{480,96,79,100},{576,192,77,105}};auto original=notes;Settings settings;assert(settings.follow==Follow::Phrase&&settings.strength==85&&settings.complexity==7&&settings.leading);
 for(unsigned mode=0;mode<5;++mode){settings.follow=Follow(mode);auto result=generate(notes,768,96,384,settings);assert(result.count&&result.count<=max_chords);for(unsigned i=0;i<result.count;++i){assert(result.chords[i].length);assert(result.chords[i].tick+result.chords[i].length<=768);}if(mode==1)for(unsigned i=0;i<result.count;++i){assert(result.chords[i].tick%96==0);}if(mode==3)assert(result.count==2);
 auto bytes=midi(result,1200,4,4);assert(bytes[8]==0&&bytes[9]==0&&bytes[11]==1);std::vector<SmfTrack> parsed;std::string why;unsigned tempo=0;assert(read_smf(bytes,parsed,why,&tempo));assert(parsed.size()==1&&tempo==1200);std::vector<std::tuple<unsigned,unsigned,unsigned,unsigned>> expected,got;for(unsigned i=0;i<result.count;++i){const auto&c=result.chords[i];for(unsigned n=0;n<c.count;++n)expected.emplace_back(c.tick,c.length,c.notes[n],c.velocity);}for(const auto&n:parsed[0].notes)got.emplace_back(n.tick,n.length,n.key,n.velocity);std::sort(expected.begin(),expected.end());std::sort(got.begin(),got.end());assert(expected==got);
 }
 settings.follow=Follow::Phrase;
 for(unsigned mask=1;mask<8;++mask){settings.complexity=uint8_t(mask);auto r=generate(notes,768,96,384,settings);assert(r.count);for(unsigned i=0;i<r.count;++i){unsigned flag=1u<<(r.chords[i].count-3);assert(mask&flag);}}
 settings.complexity=7;auto a=generate(notes,768,96,384,settings),again=generate(notes,768,96,384,settings);assert(midi(a,1200,4,4)==midi(again,1200,4,4));bool alternatives=false;for(unsigned n=1;n<32;++n){settings.variation=n;alternatives|=midi(a,1200,4,4)!=midi(generate(notes,768,96,384,settings),1200,4,4);}assert(alternatives);
 bool strengthChanges=false;for(unsigned v=0;v<24;++v){settings.variation=v;settings.strength=0;auto low=generate(notes,768,96,384,settings);settings.strength=100;auto high=generate(notes,768,96,384,settings);strengthChanges|=midi(low,1200,4,4)!=midi(high,1200,4,4);}assert(strengthChanges);
 assert(notes.size()==original.size());for(unsigned i=0;i<notes.size();++i)assert(notes[i].key==original[i].key&&notes[i].tick==original[i].tick&&notes[i].length==original[i].length);
 assert(!generate({},768,96,384,settings).count);settings.complexity=0;assert(!generate(notes,768,96,384,settings).count);
 auto data=midi(a,1200,4,4);std::string path,error;assert(!save("","test",data,path,error));assert(!save("/dev/null","test",data,path,error));auto folder=std::filesystem::temp_directory_path()/"tw-chord-export-test";std::filesystem::create_directories(folder);assert(save(folder.string(),"TW_TRAP_CHORDS_120BPM",data,path,error));auto first=path;assert(save(folder.string(),"TW_TRAP_CHORDS_120BPM",data,path,error)&&path!=first);assert(!save(folder.string(),"/..",data,path,error));std::filesystem::remove_all(folder);

 Settings progress;progress.root=2;progress.scale_mask=0x5ad;
 for(unsigned mask=1;mask<8;++mask){progress.complexity=mask;for(unsigned bars:{4u,8u}){auto r=progression(bars,384,progress);assert(r.count==bars&&r.tonic==2&&r.minor);for(unsigned n=0;n<r.count;++n){auto& c=r.chords[n];assert(c.tick==n*384&&c.length==384);assert(mask&(1u<<(c.count-3)));for(unsigned k=0;k<c.count;++k)assert(c.notes[k]<128);}std::vector<SmfTrack> parsed;std::string error;assert(read_smf(midi(r,1200,4,4),parsed,error)&&parsed.size()==1);}}
 progress.complexity=7;auto p0=midi(progression(4,384,progress),1200,4,4);bool different=false;for(unsigned n=1;n<20;++n){progress.variation=n;different|=p0!=midi(progression(4,384,progress),1200,4,4);}assert(different);
 Settings rooted;rooted.root=9;rooted.scale_mask=0x5ad;auto fixed=generate(notes,768,96,384,rooted);assert(fixed.tonic==9&&fixed.minor);
 Result invalid;invalid.count=1;invalid.chords[0].count=3;invalid.chords[0].length=0xffffffff;assert(midi(invalid,1200,4,4).empty());
 auto escape=std::filesystem::temp_directory_path()/"tw-chord-escape-test";std::filesystem::create_directories(escape/"Melodee");std::filesystem::create_directory_symlink(std::filesystem::temp_directory_path(),escape/"Melodee/MIDI");assert(!save(escape.string(),"escape",data,path,error));std::filesystem::remove_all(escape);
 std::cout<<"4/8-bar progressions, fixed/project key, five follow modes, complexity subsets, deterministic alternatives, strength, exact SMF roundtrip, source preservation and storage passed\n";
}
