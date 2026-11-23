// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop; host based on Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include "melodee/pages/common.h"
#include "melodee/pages/generator_focus.h"
#include "melodee/ui/widgets.h"
#include "melodee/ui/knobs.h"
#include "melodee/model/scales.h"
namespace melodee {
inline void draw_harmony_options(ui::Paint& p,const Snapshot& s){
    using namespace ui;const auto& h=s.ui.harmony;const auto& settings=h.settings;const auto& phrase=harmony_phrase(h);
    const auto accent=generator_accent(),mint=generator_activity();
    const char* titles[]={"CHORD GEN","BASS GEN","PLUCK GEN","MELODY GEN","HYBRID GEN"};
    Header head=header_of(s);head.left.push_back(chip(titles[std::min(4u,unsigned(h.parts.output))],accent));
    SoftKeys keys{{h.busy?"CANCEL":"ROLL",h.mode==2?"ANALYSE WAV":h.stale?"GENERATE":"NEW CHORDS",s.ui.shift?"EXPORT MIDI":"ADD TO ROLL"},-1,accent};if(h.options&&!h.busy)keys=SoftKeys{{"DONE","GENERATE",s.ui.shift?"SAVE MIDI":"LISTEN"},-1,accent};frame(p,s,keys,head);
    if(h.confirm){
        p.fill(0,190,320,50,colour::bg);p.fill(0,0,320,22,colour::bg);soft_keys(p,SoftKeys{{"CANCEL","KEEP + ADD","REPLACE"},int(h.confirm_pick),accent});
        p.fill(5,45,310,143,colour::raised,5);p.text("ROLL HAS MIDI NOTES",12,55,295,22,plain(accent,18,Weight::Bold));
        p.text("Track "+juce::String(h.confirm_track+1)+" / P"+juce::String(h.confirm_pattern+1),12,85,295,18,plain(colour::text,type::body));
        p.text("KEEP: add notes beside existing MIDI",12,111,295,17,plain(colour::text,type::small));p.text("REPLACE: remove notes, then add new MIDI",12,134,295,17,plain(colour::text,type::small));
        p.text("CANCEL leaves your Roll unchanged",12,160,295,17,plain(mint,type::small));return;
    }
    if(h.busy){
        p.fill(0,190,320,50,colour::bg);p.text("ANALYSING WHOLE WAV",9,53,300,24,plain(accent,19,Weight::Bold));p.text(h.audio_name,9,86,300,32,p.fit(h.audio_name,plain(colour::text,type::small),300,type::floor));
        p.fill(10,132,300,8,colour::raised,3);p.fill(10,132,3*h.percent,8,mint,3);p.text(juce::String(h.percent)+"%",10,148,300,27,plain(mint,23,Weight::Bold),juce::Justification::centred);
        p.text("Background analysis / B1 cancels",10,179,300,12,plain(colour::mid,type::small));return;
    }
    const auto* ch=h.result.count?&h.result.chords[h.selected%h.result.count]:nullptr;
    const char* roots[]={"C","C#","D","Eb","E","F","F#","G","Ab","A","Bb","B"};
    auto noteName=[&](unsigned pitch){return juce::String(roots[pitch%12])+juce::String(int(pitch/12)-1);};
    auto regionName=[&](const harmony::Chord& chord)->juce::String{if(h.parts.output==1)return noteName(harmony::bass_pitch(chord,h.parts.bass));if(h.parts.output==2)return noteName(48+12*h.parts.pluck.octave+chord.root);if(h.parts.output==3)return noteName(48+12*h.parts.melody.octave+chord.root);return harmony::chord_name(chord);};
    const char* labels[]={"FOLLOW MELODY","FOLLOW STRENGTH","TRIADS","SEVENTHS","NINTHS","VOICE LEADING","GENRE","INVERSION","START TICK","DURATION","TARGET TRACK","VARIATION","KEY SOURCE","GENERATOR MODE","LOOP LENGTH","WAV TRACK","WAV PAD","OUTPUT","BASS STYLE","BASS OCTAVE","BASS GATE","TOP RATE","TOP OCTAVE","TOP GATE","NEW PHRASE","PATTERN","MIDI TRACK","MIDI PATTERN","MIDI READ","MIDI PAD","RHYTHM"};
    juce::String wavFile="No WAV loaded";auto slot=s.project.tracks[h.wav_track%track_count].pads[h.wav_pad%sampler_pads].sample;if(slot<sample_slots&&s.project.samples[slot].used())wavFile=juce::String(s.project.samples[slot].path).fromLastOccurrenceOf("/",false,false);
    auto value=[&](unsigned row)->juce::String{switch(row){case 33:return h.scale_choice?juce::String(note_letter(h.mode==2&&h.wav_key_valid?h.wav_key_root:s.project.root))+" "+scales()[(h.scale_choice-1)%scales().size()].name:h.mode==2?(h.wav_key_valid?juce::String("Auto / ")+note_letter(h.wav_key_root)+(h.wav_key_minor?" minor":" major"):juce::String("Auto / WAV")):h.mode==1&&!h.follow_project?juce::String("Auto / MIDI"):juce::String("Project / ")+scales()[s.project.scale%scales().size()].name;case 0:return h.mode==0?"-":harmony::follow_name(settings.follow);case 1:return h.mode==0?"-":juce::String(settings.strength)+"%";case 2:case 3:case 4:return settings.complexity&(1<<(row-2))?"ON":"OFF";case 5:return settings.leading?"ON":"OFF";case 6:return music::harmony_genre_name(settings.genre);case 7:return ch&&h.parts.output==0?juce::String(ch->inversion):"-";case 8:return ch?juce::String(ch->tick):"-";case 9:return ch?juce::String(ch->length):"-";case 10:return juce::String(h.target+1);case 11:return h.mode==2?"-":juce::String(settings.variation);case 12:return h.mode==2?"WHOLE AUDIO":h.mode==0||h.follow_project?"PROJECT":"DETECT MIDI";case 13:return h.mode==0?"PROGRESSION":h.mode==1?"FOLLOW MIDI":"WHOLE WAV";case 14:return h.mode==0?juce::String(h.bars)+" BARS":h.mode==2?wavFile:juce::String("ROLL LENGTH");case 15:return juce::String(h.wav_track+1);case 16:return juce::String::charToString(char('A'+h.wav_pad/16))+juce::String(h.wav_pad%16+1).paddedLeft('0',2);case 17:return harmony::output_name(h.parts.output);case 18:return h.parts.output==1?harmony::bass_style(h.parts.bass.style):"-";case 19:return h.parts.output==1?juce::String("C")+juce::String(h.parts.bass.octave):juce::String("-");case 20:return h.parts.output==1?juce::String(h.parts.bass.gate)+"%":juce::String("-");case 21:return h.parts.output>=2?harmony::phrase_rate(phrase.rate):"-";case 22:return h.parts.output>=2?juce::String("C")+juce::String(phrase.octave+3):juce::String("-");case 23:return h.parts.output>=2?juce::String(phrase.gate)+"%":juce::String("-");case 24:return h.parts.output>=2?juce::String(phrase.variation):juce::String("-");case 25:return h.parts.output>=2?harmony::phrase_pattern(phrase.pattern,harmony_edits_melody(h),settings.genre):"-";case 26:return juce::String(h.midi_track+1)+" "+juce::String(s.project.tracks[h.midi_track].name);case 27:return "P"+juce::String(h.midi_pattern+1);case 28:return h.midi_read?"PAD PITCHES":"MIDI NOTES";case 30:return !harmony::output_rhythm(h.parts)&&h.settings.genre&&h.parts.output!=1?juce::String("Genre Auto"):juce::String(harmony::bass_rhythm(harmony::output_rhythm(h.parts)));default:return juce::String::charToString(char('A'+h.midi_pad/16))+juce::String(h.midi_pad%16+1).paddedLeft('0',2);}};
    auto rows=harmony_rows(h);unsigned selected=0;for(unsigned i=0;i<rows.count;++i)if(rows.at[i]==h.row)selected=i;
    p.fill(0,0,320,240,colour::bg);soft_keys(p,SoftKeys{{"DONE","GENERATE",s.ui.shift?"SAVE MIDI":"LISTEN"},-1,accent});
    p.text("GENERATOR SETTINGS",8,28,304,22,plain(colour::text,17,Weight::Bold));
    juce::String context=titles[std::min(4u,unsigned(h.parts.output))];if(ch)context+=" / "+regionName(*ch);
    p.text(context,8,61,304,23,p.fit(context,plain(mint,16,Weight::Bold),304,14));
    generator_focus(p,h.row==33?juce::String("Scale"):juce::String(labels[h.row]),value(h.row),selected,rows.count,s.now,h.field_changed_at);
    p.text("K1: setting / K2 or Data: change",9,201,302,18,plain(mint,13,Weight::Bold));
    p.text("Push: listen / K3: chord region",9,221,302,16,plain(colour::mid,12,Weight::Bold));

}
inline void draw_harmony(ui::Paint& p, const Snapshot& s) {
    using namespace ui;
    const auto& h=s.ui.harmony;
    if(h.options||h.confirm||h.busy){draw_harmony_options(p,s);return;}
    const auto purple=generator_accent(),mint=generator_activity();
    const char* steps[]={"SOURCE","OUTPUT","TARGET","PREVIEW","ADD"};
    const char* titles[]={"1. Choose your source","2. What do you want to make?","3. Choose your instrument","4. Generate and listen","5. Add your notes"};
    p.fill(0,0,320,240,colour::bg);
    soft_keys(p,SoftKeys{{h.step==0&&h.row==13?"ROLL":"BACK",h.step==4?"ADD TO ROLL":"NEXT",s.ui.shift&&h.step==4?"SAVE MIDI":"OPTIONS"},-1,purple});
    p.text(h.parts.output==1?"BASS GENERATOR":"MIDI GENERATOR",8,26,251,18,plain(colour::text,14,Weight::Bold));
    p.text(juce::String(h.step+1)+" / 5",268,28,44,15,plain(mint,11,Weight::Bold),juce::Justification::centredRight);
    for(unsigned n=0;n<5;++n){float x=8+n*61;p.fill(x,50,59,18,n==h.step?purple:colour::raised,3);p.text(steps[n],x+1,52,57,14,p.fit(steps[n],plain(n==h.step?colour::bg:colour::mid,11,Weight::Bold),57,type::floor),juce::Justification::centred);}
    p.text(h.parts.output==1&&h.step>=3?"Bass roots / octave and rhythm":titles[h.step],9,74,302,20,p.fit(titles[h.step],plain(mint,15,Weight::Bold),302,type::floor));
    const char* names[]={"Follow","Strength","Triads","Sevenths","Ninths","Leading","Genre","Inversion","Start","Length","Instrument track","Variation","Key","Source type","Loop length","WAV track","WAV pad","Output","Bass style","Bass octave","Bass gate","Rate","Octave","Gate","New phrase","Pattern","MIDI track","MIDI pattern","Read notes from","MIDI pad","Rhythm"};
    auto padName=[](unsigned pad){return juce::String::charToString(char('A'+pad/16))+juce::String(pad%16+1).paddedLeft('0',2);};
    auto trackName=[&](unsigned t){return "T"+juce::String(t+1)+" "+juce::String(s.project.tracks[t].name);};
    auto value=[&](unsigned row)->juce::String{
        switch(row){
        case 10:return trackName(h.target);
        case 13:return h.mode==0?"Project key / scale":h.mode==1?"MIDI Roll":"WAV sample";
        case 14:return juce::String(h.bars)+" bars";
        case 15:return trackName(h.wav_track);
        case 16:return padName(h.wav_pad);
        case 33:return h.scale_choice?juce::String(note_letter(h.mode==2&&h.wav_key_valid?h.wav_key_root:s.project.root))+" "+scales()[(h.scale_choice-1)%scales().size()].name:h.mode==2?(h.wav_key_valid?juce::String("Auto / ")+note_letter(h.wav_key_root)+(h.wav_key_minor?" minor":" major"):juce::String("Auto / WAV")):h.mode==1&&!h.follow_project?juce::String("Auto / MIDI"):juce::String("Project / ")+scales()[s.project.scale%scales().size()].name;
        case 6:return music::harmony_genre_name(h.settings.genre);
        case 17:return harmony::output_name(h.parts.output);
        case 19:return juce::String("C")+juce::String(h.parts.bass.octave)+(h.parts.bass.octave==1?" / Low 808":"");
        case 30:return !harmony::output_rhythm(h.parts)&&h.settings.genre&&h.parts.output!=1?juce::String("Genre Auto"):juce::String(harmony::bass_rhythm(harmony::output_rhythm(h.parts)));
        case 26:return trackName(h.midi_track);
        case 27:return "P"+juce::String(h.midi_pattern+1);
        case 28:return h.midi_read?"One pitched sampler pad":"All MIDI note keys";
        default:return padName(h.midi_pad);
        }
    };
    juce::String selectedValue;
    if(h.step<3){
        auto rows=harmony_step_rows(h);unsigned index=0;
        for(unsigned n=0;n<rows.count;++n)if(rows.at[n]==h.row)index=n;
        generator_focus(p,h.row==33?juce::String("Scale"):juce::String(names[h.row]),value(h.row),index,rows.count,s.now,h.field_changed_at);
        selectedValue=value(h.row);
        juce::String hint="Data: choose / Push: confirm";
        if(h.step==0&&h.mode==2){auto slot=s.project.tracks[h.wav_track].pads[h.wav_pad].sample;hint=slot<sample_slots&&s.project.samples[slot].used()?juce::String(s.project.samples[slot].name):"No sample loaded on this pad";}
        if(h.step==0&&h.mode==2&&h.wav_key_valid)hint="Detected: "+juce::String(note_letter(h.wav_key_root))+(h.wav_key_minor?" minor":" major");
        p.text(hint,9,218,302,16,p.fit(hint,plain(mint,12,Weight::Bold),302,type::floor));
    }else{
        const char* actions[]={"Generate notes",h.parts.output==1?"Listen to bass rhythm":"Listen to rhythm","Try another","More options"};
        const char* finalActions[]={"Add to Roll",h.parts.output==1?"Listen to bass rhythm":"Listen to rhythm","Save MIDI copy","More options"};
        selectedValue=h.step==3?actions[h.choice]:finalActions[h.choice];
        if(h.step==3&&h.choice==2&&h.mode==2&&h.parts.output<2)selectedValue="Reanalyse WAV";
        p.fill(8,97,304,31,purple.withAlpha(.18f),4);p.ring(8,97,304,31,purple,4);
        p.text(selectedValue,14,100,292,24,p.fit(selectedValue,plain(colour::text,21,Weight::Bold),292,17));
        if(h.stale||!h.result.count){
            p.text("Choose Generate notes, then Push",10,140,300,22,plain(mint,15,Weight::Bold));
            p.text("Your WAV / MIDI source stays selected",10,167,300,20,p.fit("Your WAV / MIDI source stays selected",plain(colour::text,13,Weight::Bold),300,type::floor));
        }else{
            const unsigned selected=h.selected%h.result.count,first=(selected/4)*4;
            for(unsigned n=0;n<4&&first+n<h.result.count;++n){const auto& chord=h.result.chords[first+n];bool on=first+n==selected;float x=8+n*77;
                p.fill(x,133,73,24,on?purple:colour::raised,3);p.ring(x,133,73,24,purple.withAlpha(on?1.f:.4f),3);
                auto pitch=harmony::bass_pitch(chord,h.parts.bass);auto name=h.parts.output==1?juce::String(note_letter(pitch%12))+juce::String(int(pitch/12)-1):juce::String(harmony::chord_name(chord));p.text(name,x+3,136,67,18,p.fit(name,plain(on?colour::bg:colour::text,14,Weight::Bold),67,type::floor),juce::Justification::centred);
            }
            // A map of the actual generated harmonic voicings, not a decorative waveform.
            // Pluck/Melody/Hybrid still show their harmonic foundation; the Roll owns note editing.
            const float gx=8,gy=162,gw=304,gh=34;
            p.fill(gx,gy,gw,gh,colour::bg,2);p.ring(gx,gy,gw,gh,purple.withAlpha(.3f),2);
            unsigned low=127,high=0;uint32_t from=h.result.chords[first].tick,to=from+1;
            for(unsigned n=first;n<std::min<unsigned>(h.result.count,first+4);++n){const auto& chord=h.result.chords[n];to=std::max(to,chord.tick+chord.length);for(unsigned k=0;k<(h.parts.output==1?1u:chord.count);++k){unsigned pitch=h.parts.output==1?harmony::bass_pitch(chord,h.parts.bass):chord.notes[k];low=std::min(low,pitch);high=std::max(high,pitch);}}
            if(high<low){low=48;high=72;}unsigned range=std::max(12u,high-low+1);
            for(unsigned n=0;n<4;++n)p.fill(gx+n*gw/4,gy,1,gh,purple.withAlpha(.15f));
            for(unsigned n=first;n<std::min<unsigned>(h.result.count,first+4);++n){const auto& chord=h.result.chords[n];
                float nx=gx+2+float(chord.tick-from)/float(to-from)*(gw-4),nw=std::max(1.f,float(chord.length)/float(to-from)*(gw-4)-2);
                for(unsigned k=0;k<(h.parts.output==1?1u:chord.count);++k){unsigned pitch=h.parts.output==1?harmony::bass_pitch(chord,h.parts.bass):chord.notes[k];float ny=gy+gh-4-float(pitch-low)/range*(gh-6);p.fill(nx,ny,nw,2,n==selected?mint:purple.withAlpha(.75f),1);}
            }
        }
        juce::String source=h.mode==2?"WAV "+padName(h.wav_pad):h.mode==1?"MIDI T"+juce::String(h.midi_track+1)+" P"+juce::String(h.midi_pattern+1):"PROJECT KEY";
        if(h.result.count&&!h.stale)source+=" / "+juce::String(note_letter(h.result.tonic))+(h.scale_choice?" "+juce::String(scales()[(h.scale_choice-1)%scales().size()].name):h.result.minor?" minor":" major");
        source+=h.parts.output==1?" / C"+juce::String(h.parts.bass.octave)+" / "+harmony::bass_rhythm(h.parts.bass.rhythm):" / "+juce::String(harmony::output_name(h.parts.output));
        if(h.step==4)source="To "+trackName(h.target)+" / P"+juce::String(s.project.tracks[h.target].pattern+1);
        p.text(source,10,199,300,18,p.fit(source,plain(mint,12,Weight::Bold),300,type::floor));
        if(h.rhythm_preview_count)p.text("PLAYING",229,27,83,18,plain(mint,12,Weight::Bold));
    }
    if(h.step<3){p.text("K1: setting / K2 or Data: change",9,198,302,18,plain(purple,13,Weight::Bold));}
    else {p.text("Data: action / Push: run / K3: chord",9,222,302,16,plain(colour::mid,12,Weight::Bold));}

}

}
