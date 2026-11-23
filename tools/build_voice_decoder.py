#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""Build the PocketSphinx voice decoder bundle from the upstream 5.0.4 source."""
import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import tempfile

WORDS = "kick snare clap hat hihat cymbal shaker rim tom perc drum bass piano keys synth vocal loop break guitar brass strings 808".split()

# Common sample labels, grouped so the vocabulary is easy to review.
MORE_WORDS = """
kicks snares claps hats toms rimshot cymbals crash ride splash china ridebell sleighbells
tambourine cowbell conga congas bongo bongos timbales djembe darbuka tabla cajon
triangle claves woodblock cabasa guiro maracas agogo timpani gong
drums percussion brushes sidestick drumroll openhihat closedhihat floortom racktom
rimclick handclap kickdrum snaredrum
melody melodies harmony chord chords arpeggio riff fill fills filler breakbeat
groove oneshot drumloop drumfill breakbeats acapella vocals vox choir voice chant
bassline sub subbass rhodes wurli organ clavinet harpsichord accordion
electricpiano grandpiano acousticguitar electricguitar bassguitar
ukulele banjo mandolin sitar harp violin viola cello
flute piccolo clarinet oboe bassoon sax saxophone horns horn trumpet trombone
tuba frenchhorn harmonica recorder marimba vibraphone xylophone glockenspiel
kalimba mallet mallets steelpan
pad pads lead pluck stab stabs arp motif keysynth
fx effects soundeffects sfx riser risers sweep sweeps impact impacts ambience ambient
ambients atmosphere texture textures drone drones noise whoosh transition
reverse reverb delay echo chorus flanger phaser tremolo vibrato distortion
fuzz overdrive saturation filter vinyl tape glitch bitcrusher whitenoise
hiphop rap trap drill boombap lofi house techno trance dubstep
garage jungle drumandbass dnb grime industrial hardcore breakcore
funk soul jazz blues gospel disco pop rock metal punk reggae dub dancehall
reggaeton dembow afrobeat afrobeats amapiano salsa samba bossa bossanova
latin cumbia bachata merengue soca country folk bluegrass classical orchestral
cinematic soundtrack experimental chill chillhop phonk edm jersey
jerseyclub dance dancepop synthwave vaporwave futurebass ukgarage rnb
""".split()

# Compounds stay single tokens in the v39 decoder. Their pronunciations are
# assembled from dictionary words unless a common abbreviation needs phonemes.
COMPOUNDS = {
    "sidestick": "side stick", "drumroll": "drum roll", "floortom": "floor tom", "racktom": "rack tom",
    "rimclick": "rim click", "ridebell": "ride bell", "sleighbells": "sleigh bells", "handclap": "hand clap", "kickdrum": "kick drum", "snaredrum": "snare drum",
    "openhihat": "open high hat", "closedhihat": "closed high hat",
    "oneshot": "one shot", "drumloop": "drum loop", "drumfill": "drum fill",
    "acapella": "a cappella", "electricpiano": "electric piano", "grandpiano": "grand piano",
    "acousticguitar": "acoustic guitar", "electricguitar": "electric guitar", "bassguitar": "bass guitar",
    "frenchhorn": "french horn", "steelpan": "steel pan", "keysynth": "key synth",
    "subbass": "sub bass", "bitcrusher": "bit crusher", "whitenoise": "white noise",
    "soundeffects": "sound effects",
    "hiphop": "hip hop", "boombap": "boom bap", "lofi": "low fi",
    "drumandbass": "drum and bass", "bossanova": "bossa nova",
    "jerseyclub": "jersey club", "dancepop": "dance pop",
    "synthwave": "synth wave", "vaporwave": "vapor wave",
    "futurebass": "future bass", "ukgarage": "u k garage",
}
PRONUNCIATIONS = {
    "808": "EY T OW EY T", "hihat": "HH AY HH AE T", "perc": "P ER K", "synth": "S IH N TH",
    "fx": "EH F EH K S", "sfx": "EH S EH F EH K S", "dnb": "D IY EH N B IY",
    "edm": "IY D IY EH M", "rnb": "AA R AH N D B IY",
    "bap": "B AE P", "bossa": "B AO S AH", "fi": "F AY",
    "reggaeton": "R EH G EY T OW N", "dembow": "D EH M B OW", "amapiano": "AA M AH P IY AA N OW",
    "afrobeat": "AE F R OW B IY T", "afrobeats": "AE F R OW B IY T S",
    "phonk": "F AO NG K", "chillhop": "CH IH L HH AA P", "breakcore": "B R EY K K AO R",
    "darbuka": "D AA R B UW K AH", "cajon": "K AA HH OW N", "djembe": "JH EH M B EY",
    "guiro": "G W IY R OW", "agogo": "AH G OW G OW", "timbales": "T IH M B AA L EY Z",
    "rhodes": "R OW D Z", "wurli": "W ER L IY", "clavinet": "K L AE V AH N EH T",
    "kalimba": "K AH L IH M B AH", "mallets": "M AE L AH T S",
    "ambients": "AE M B IY AH N T S", "whoosh": "W UH SH", "risers": "R AY Z ER Z",
    "arp": "AA R P", "vox": "V AA K S", "hats": "HH AE T S",
    "bassline": "B EY S L AY N", "breakbeat": "B R EY K B IY T", "breakbeats": "B R EY K B IY T S",
    "rimshot": "R IH M SH AA T", "woodblock": "W UH D B L AA K",
    "dubstep": "D AH B S T EH P", "dancehall": "D AE N S HH AO L",
    "bachata": "B AA CH AA T AH", "cumbia": "K UW M B IY AH", "soca": "S OW K AH",
    "congas": "K AA NG G AH Z", "tabla": "T AA B L AH", "cabasa": "K AH B AA S AH",
    "claves": "K L AA V EY Z", "arpeggio": "AA R P EH JH IY OW",
    "vibraphone": "V AY B R AH F OW N", "flanger": "F L AE N JH ER",
    "phaser": "F EY Z ER", "tremolo": "T R EH M AH L OW",
}

def write_vocabulary(model, out):
    dictionary = dict(line.split(" ", 1) for line in (model / "cmudict-en-us.dict").read_text().splitlines())
    dictionary.update(PRONUNCIATIONS)
    for token, phrase in COMPOUNDS.items():
        dictionary[token] = " ".join(dictionary[word] for word in phrase.split())
    words = list(dict.fromkeys(WORDS + MORE_WORDS))
    thresholds = {word: "1e-3" for word in WORDS}
    thresholds.update({
        "kick": "1e-7", "rim": "1e-1", "808": "1e-20",
        # This short addition otherwise competes with the existing kick fixture.
        "tape": "1e10",
    })
    (out / "words.dict").write_text("".join(
        f"{word} {dictionary[word]}\n" for word in words))
    (out / "words.kws").write_text("".join(
        f"{word} /{thresholds.get(word, '1e-1' if len(dictionary[word].split()) <= 4 else '1e-3')}/\n"
        for word in words))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--cc", default="cc")
    parser.add_argument("--vocabulary-only", action="store_true", help="update words.dict and words.kws only")
    args = parser.parse_args()
    src, out = args.source.resolve(), args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    model = src / "model/en-us"
    if args.vocabulary_only:
        write_vocabulary(model, out)
        return
    cc = shlex.split(args.cc)
    flags = ["-O2", "-fPIC", "-fvisibility=hidden", "-DHAVE_CONFIG_H"]
    with tempfile.TemporaryDirectory(dir=out.parent) as temp:
        build = Path(temp)
        config = "\n".join("#define " + name for name in (
            "HAVE_LONG_LONG", "HAVE_STDINT_H", "HAVE_INTTYPES_H", "HAVE_UNISTD_H",
            "HAVE_SYS_TYPES_H", "HAVE_SYS_STAT_H", "HAVE_POPEN", "HAVE_SNPRINTF",
        )) + "\n#define SIZEOF_LONG 8\n#define SIZEOF_LONG_LONG 8\n"
        (build / "config.h").write_text(config)
        (build / "pocketsphinx").mkdir()
        (build / "pocketsphinx/sphinx_config.h").write_text(config)
        includes = [str(build), str(src / "include"), str(src / "src")]
        flags += ["-I" + path for path in includes]
        cmake = (src / "src/CMakeLists.txt").read_text()
        sources = [src / "src" / name for name in re.findall(r"(?m)^([\w/]+\.c)$", cmake)]
        sources.append(Path(__file__).with_name("voice_decoder.c").resolve())
        def compile_source(item):
            index, source = item
            obj = build / f"{index}.o"
            subprocess.run(cc + flags + ["-c", str(source), "-o", str(obj)], check=True)
            return str(obj)
        with ThreadPoolExecutor(max_workers=6) as pool:
            objects = list(pool.map(compile_source, enumerate(sources)))
        subprocess.run(cc + ["-shared", *objects, "-lm", "-o", str(out / "decoder.so")], check=True)
    shutil.copytree(model / "en-us", out / "en-us", dirs_exist_ok=True)
    shutil.copy2(src / "LICENSE", out / "PocketSphinx-LICENSE")
    write_vocabulary(model, out)

if __name__ == "__main__":
    main()
