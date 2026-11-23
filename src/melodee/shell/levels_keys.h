// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (C) 2026 Kerem Kilic (Ellic Studio)
#pragma once
#include "melodee/model/progression.h"
#include "melodee/shell/ui_state.h"
#include "surface/controls.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

// 16 Levels held: B1-B3 are its keys, and they follow the type (K1 still turns every type).
//   any type but Chords   B1 SCALE, B2 CHORDS: that type, and 16 Levels on, in one press
//   Chords                B1 SCALE, B2 FILE, B3 SET
// FILE lists the card's progression files: the encoder moves, Push or B3 PICK takes the one in hand, B1 goes back.
// SET makes a chord of your own: tap the pad it goes on, then K1 ROOT, K2 CHORD and K3 INVERSION set it (tap the
// pad to hear it). B3 DONE saves the progression to <card>/Melodee/Progressions (a new one is Mine 1, Mine 2 ...)
// and waits for the next pad; B1 BACK puts the chord back as it was, and from the pads leaves SET.
// A track on Drums pads has no keys (another pitch is another drum), nor an audio track (its pads play nothing).
// Pure, for the shell, the screen and the tests; the jobs are shell/levels_keys.cpp's.
namespace melodee {

class Context;

enum class LevelsKey : uint8_t { None, Scale, Chords, File, Set };
inline const char* levels_key_name(LevelsKey k) {
    static const char* const names[] {"", "SCALE", "CHORDS", "FILE", "SET"};
    return names[unsigned(k) % 5];
}
struct LevelsKeys {
    std::array<LevelsKey, 3> key {};
    int lit = -1; // the key whose type the pads play now, -1 none
};
inline bool levels_chords_on(const Track& t) {
    return t.levels && LevelsKind(t.levels_kind % unsigned(LevelsKind::Count)) == LevelsKind::Chords;
}
inline LevelsKeys levels_keys(const Track& t) {
    LevelsKeys k;
    if (!t.used() || t.is_audio() || drum_pads(t)) return k;
    const auto kind = LevelsKind(t.levels_kind % unsigned(LevelsKind::Count));
    k.key[0] = LevelsKey::Scale;
    if (kind == LevelsKind::Chords) { k.key[1] = LevelsKey::File; k.key[2] = LevelsKey::Set; }
    else {
        k.key[1] = LevelsKey::Chords;
        if (t.levels && kind == LevelsKind::Scale) k.lit = 0;
    }
    return k;
}

// The sheets FILE and SET open (UiState::levels_sheet). They stay when 16 Levels is let go.
enum LevelsSheet : uint8_t { levels_sheet_none = 0, levels_sheet_files = 1, levels_sheet_pad = 2, levels_sheet_chord = 3 };

// FILE's rows: the first of `rows` shown with the cursor two from the top where the list allows.
constexpr unsigned prog_rows_shown = 6;
inline unsigned prog_rows_first(unsigned cursor, unsigned count, unsigned rows = prog_rows_shown) {
    if (count <= rows) return 0;
    return std::min(cursor < 2 ? 0u : cursor - 2, count - rows);
}

// SET's chords: a name as progression files write it ("min9", "Maj7": pads show them short, model/progression.h)
// and its notes in semitones over the root.
struct BuiltChord { const char* name; uint8_t count; std::array<uint8_t, 6> steps; };
inline const std::array<BuiltChord, 24>& built_chords() {
    static const std::array<BuiltChord, 24> chords {{
        {"Maj", 3, {0, 4, 7}}, {"min", 3, {0, 3, 7}}, {"7", 4, {0, 4, 7, 10}}, {"Maj7", 4, {0, 4, 7, 11}}, {"min7", 4, {0, 3, 7, 10}},
        {"6", 4, {0, 4, 7, 9}}, {"min6", 4, {0, 3, 7, 9}}, {"9", 5, {0, 4, 7, 10, 14}}, {"Maj9", 5, {0, 4, 7, 11, 14}},
        {"min9", 5, {0, 3, 7, 10, 14}}, {"min11", 6, {0, 3, 7, 10, 14, 17}}, {"13", 6, {0, 4, 7, 10, 14, 21}}, {"6/9", 5, {0, 4, 7, 9, 14}},
        {"Maj(add9)", 4, {0, 4, 7, 14}}, {"min(add9)", 4, {0, 3, 7, 14}}, {"sus2", 3, {0, 2, 7}}, {"sus4", 3, {0, 5, 7}},
        {"7sus4", 4, {0, 5, 7, 10}}, {"9sus4", 5, {0, 5, 7, 10, 14}}, {"dim", 3, {0, 3, 6}}, {"dim7", 4, {0, 3, 6, 9}},
        {"min7b5", 4, {0, 3, 6, 10}}, {"aug", 3, {0, 4, 8}}, {"5", 3, {0, 7, 12}},
    }};
    return chords;
}
constexpr unsigned built_chord_count = 24;
constexpr unsigned built_inversions = 4; // Root, 1st, 2nd, 3rd (model/chords.h chord_invert_name)
// The chord a card names: root `root` (a pitch class) in the octave of C3, `type` of built_chords(), its lowest
// `inversion` notes an octave up (one fewer than it has notes at most). Named with a slash bass when inverted.
inline ProgChord build_prog_chord(unsigned root, unsigned type, unsigned inversion) {
    const BuiltChord& shape = built_chords()[type % built_chord_count];
    std::array<int, notes_per_step> keys {};
    unsigned n = 0;
    for (unsigned k = 0; k < shape.count && n < notes_per_step; ++k) keys[n++] = 48 + int(root % 12) + shape.steps[k];
    for (unsigned k = 0; k < std::min<unsigned>(inversion, n - 1); ++k) { keys[0] += 12; sort_notes(keys, n); }
    ProgChord chord;
    for (unsigned k = 0; k < n; ++k) chord.notes[k] = uint8_t(std::clamp(keys[k], 0, 127));
    chord.count = uint8_t(n);
    std::string name = std::string(note_letter(root % 12)) + shape.name;
    if (unsigned(keys[0]) % 12 != root % 12) name += std::string("/") + note_letter(unsigned(keys[0]) % 12);
    std::snprintf(chord.name, sizeof(chord.name), "%s", name.c_str());
    return chord;
}
// What the cards start on for a chord already on the pad: the root, type and inversion that give its pitch classes
// with its lowest note in the bass; false when no chord of the table does (the cards then start on `root` and the
// chord stays as it is until a card is turned).
struct BuiltPick { uint8_t root = 0, type = 0, inversion = 0; };
inline bool find_built_chord(const ProgChord& chord, BuiltPick& out) {
    if (!chord.used()) return false;
    unsigned mask = 0;
    for (unsigned k = 0; k < chord.count; ++k) mask |= 1u << (chord.notes[k] % 12);
    const unsigned bass = chord.notes[0] % 12;
    for (unsigned turn = 0; turn < 12; ++turn) { // the bass as the root first
        const unsigned root = (bass + turn) % 12;
        for (unsigned type = 0; type < built_chord_count; ++type) {
            const BuiltChord& shape = built_chords()[type];
            unsigned bits = 0;
            for (unsigned k = 0; k < shape.count; ++k) bits |= 1u << ((root + shape.steps[k]) % 12);
            if (bits != mask) continue;
            for (unsigned k = 0; k < shape.count && k < built_inversions; ++k)
                if ((root + shape.steps[k]) % 12 == bass) { out = {uint8_t(root), uint8_t(type), uint8_t(k)}; return true; }
        }
    }
    return false;
}

// MIDI thread (shell/levels_keys.cpp).
void levels_key(Context& c, unsigned key);                 // B1-B3 while 16 Levels is held
bool levels_sheet_button(Context& c, SurfaceButton button); // a press while FILE or SET is open; false: the button's own job (Play, Stop)
void levels_sheet_wheel(Context& c, int delta);            // FILE: the row in hand
void levels_set_pad(Context& c, unsigned pad);           // SET: the pad the chord goes on (the pad still plays)
void levels_chord_card(Context& c, unsigned card, int value); // SET's K1 ROOT, K2 CHORD, K3 INVERSION
void levels_chords_on_now(Context& c);                     // the type Chords, 16 Levels on, the card's first progression when the track has none

} // namespace melodee
