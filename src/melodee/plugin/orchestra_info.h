// SPDX-License-Identifier: AGPL-3.0-only
// Tape Workshop instrument metadata; host based on Melodee by Kerem Kiliç (Ellic Studio).
#pragma once
#include <array>
namespace melodee {
inline constexpr std::array<unsigned,4> orchestra_first {0,5,9,13};
inline constexpr std::array<unsigned,4> orchestra_count {5,4,4,4};
inline constexpr std::array<const char*,17> orchestra_instruments {
    "Violin Section","Solo Violin","Viola Section","Cello Section","Contrabass",
    "French Horn","Trumpet","Trombone","Tuba","Flute","Oboe","Clarinet","Bassoon",
    "Timpani","Marimba","Glockenspiel","Tubular Bells"
};
inline constexpr std::array<const char*,17> orchestra_short_names {
    "Violins","Solo Vln","Violas","Cellos","Basses","Horn","Trumpet","Trombone","Tuba",
    "Flute","Oboe","Clarinet","Bassoon","Timpani","Marimba","Glock","Bells"
};
inline constexpr std::array<const char*,48> orchestra_parameter_names {
    "Articulation","Strings","Brass","Woodwinds","Percussion","Ensemble","Detune","Stereo Width",
    "Attack","Decay","Sustain","Release","Cutoff","Resonance","Vibrato","Vibrato Rate",
    "Expression","Reverb","Tone","Output","Motion Rate","Motion Depth","Motion Shape","Motion Target",
    "Room Echo","Echo Time","Feedback","Solo Legato","Glide Time","Max Glide",
    "String Player","Brass Player","Wind Player","Perc Player","Dynamics", "Melody Play", "New Melody", "Key", "Scale", "Note Speed", "Octave", "Note Range", "Pattern Steps", "Note Gate", "Swing", "Density", "Melody Velocity", "Follow Project"
};
inline constexpr std::array<const char*,32> orchestra_programs {
    "Symphony Hall","Chamber Orchestra","Soft Orchestra","Orchestral Swell",
    "Lush Strings","Solo Violin","Warm Violas","Deep Cellos","Contrabass Bow",
    "French Horns","Heroic Trumpet","Low Trombones","Deep Tuba",
    "Flute Melody","Expressive Oboe","Warm Clarinet","Dark Bassoon",
    "Timpani Hits","Marimba Patterns","Glockenspiel","Concert Bells",
    "Short Strings","Short Brass","Short Woodwinds","Orchestral Stabs",
    "Solo Legato Violin","Legato Horn","Legato Flute","Cinematic Motion",
    "Echo Orchestra","Dark Underscore","Grand Finale"
};
inline unsigned orchestra_player(unsigned family,float value) {
    return orchestra_first[family]+unsigned(value*float(orchestra_count[family]-1)+.5f);
}
}
