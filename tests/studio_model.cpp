// SPDX-License-Identifier: AGPL-3.0-only
#include "melodee/shell/studio_model.h"
#include <cassert>
#include <iostream>
using namespace melodee;
template<class F> void rejected(F run){bool failed=false;try{run();}catch(const std::runtime_error&){failed=true;}assert(failed);}
int main(){
    Pattern pattern;pattern.length=1536;Event note;note.tick=24;note.key=60;note.length=24;note.velocity=99;note.ratchet=3;note.chance=45;note.pitch=-2;pattern.add(note);
    const auto rev=studio_revision(pattern);
    StudioNoteChange change;change.kind=StudioNoteChange::Kind::Update;change.index=0;change.tick=48;change.key=62;change.length=48;change.velocity=111;
    auto next=studio_notes(pattern,rev,{change},127);assert(pattern.events[0]==note);assert(next.events[0].tick==48&&next.events[0].key==62&&next.events[0].velocity==111);assert(next.events[0].ratchet==3&&next.events[0].chance==45&&next.events[0].pitch==-2);
    rejected([&]{studio_notes(next,rev,{change},127);});
    auto bad=change;bad.tick=1535;bad.length=24;rejected([&]{studio_notes(pattern,rev,{change,bad},127);});assert(pattern.events[0]==note);
    bad=change;bad.key=164;rejected([&]{studio_notes(pattern,rev,{bad},163);});bad.key=0;rejected([&]{studio_notes(pattern,rev,{bad},163);});
    bad=change;bad.velocity=128;rejected([&]{studio_notes(pattern,rev,{bad},127);});bad=change;bad.length=0;rejected([&]{studio_notes(pattern,rev,{bad},127);});
    rejected([&]{studio_notes(pattern,rev,{change,change},127);});
    StudioNoteChange add;add.tick=0;add.key=36;add.length=24;
    auto added=studio_notes(pattern,rev,{add},163);assert(added.events.size()==2&&added.events[0].key==36);
    auto copied=add;copied.carry_play=true;copied.play=note;copied.tick=96;copied.play.ratchet=6;
    auto pasted=studio_notes(pattern,rev,{copied},127);assert(pasted.events.back().ratchet==6&&pasted.events.back().chance==45&&pasted.events.back().pitch==-2);
    auto head=change;head.tick=24;head.length=12;auto tail=copied;tail.tick=36;tail.length=12;
    auto split=studio_notes(pattern,rev,{head,tail},127);assert(split.events.size()==2&&split.events[0].length==12&&split.events[1].tick==36&&split.events[1].chance==45);
    copied.play.ratchet=0;rejected([&]{studio_notes(pattern,rev,{copied},127);});assert(pattern.events[0]==note);
    StudioNoteChange remove;remove.kind=StudioNoteChange::Kind::Delete;remove.index=0;assert(studio_notes(pattern,rev,{remove},127).events.empty());
    Song song;Clip first;first.track=1;first.pattern=7;first.bars=4;auto arranged=studio_clip(song,studio_revision(song),-1,&first);assert(song.count==0&&arranged.count==1&&arranged.clips[0].pattern==7);
    Clip overlap=first;overlap.bar=2;rejected([&]{studio_clip(arranged,studio_revision(arranged),-1,&overlap);});assert(arranged.count==1);
    Clip moved=first;moved.bar=6;auto movedSong=studio_clip(arranged,studio_revision(arranged),0,&moved);assert(movedSong.clips[0].bar==6&&arranged.clips[0].bar==0);
    rejected([&]{studio_clip(movedSong,studio_revision(arranged),0,nullptr);});
    assert(studio_clip(movedSong,studio_revision(movedSong),0,nullptr).count==0);
    Clip outside=first;outside.bar=998;outside.bars=2;rejected([&]{studio_clip(song,studio_revision(song),-1,&outside);});
    Song full;full.count=song_clips;for(unsigned i=0;i<song_clips;i++){full.clips[i].bar=uint16_t(i*2);full.clips[i].bars=1;}Clip extra;extra.track=1;extra.bar=0;rejected([&]{studio_clip(full,studio_revision(full),-1,&extra);});assert(full.count==song_clips);
    std::cout<<"PASS: transactional MIDI edits, metadata preservation, stale revisions, range limits, clip movement, overlap protection and capacity failure without data loss\n";
}
