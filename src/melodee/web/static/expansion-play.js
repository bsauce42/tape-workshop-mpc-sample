// SPDX-License-Identifier: AGPL-3.0-only
'use strict';
(() => {
    class PadPlayer {
        constructor(context) { this.context = context; this.voices = []; this.generation = 0; }
        release(voice) {
            const now = voice.ctx.currentTime;
            voice.gain.gain.cancelScheduledValues(now);
            voice.gain.gain.setValueAtTime(voice.gain.gain.value, now);
            voice.gain.gain.linearRampToValueAtTime(0, now + .005);
            voice.source.stop(now + .006);
        }
        async play(buffer) {
            const ctx = this.context(), generation = this.generation;
            await ctx.resume();
            if (generation !== this.generation) return;
            if (this.voices.length >= 16) this.release(this.voices.shift());
            const source = ctx.createBufferSource(), gain = ctx.createGain();
            source.buffer = buffer; gain.gain.value = .7;
            source.connect(gain); gain.connect(ctx.destination);
            const voice = {source, gain, ctx};
            this.voices.push(voice);
            source.onended = () => {
                source.disconnect(); gain.disconnect();
                this.voices = this.voices.filter(item => item !== voice);
            };
            source.start();
        }
        stop() { this.generation++; for (const voice of this.voices.splice(0)) this.release(voice); }
    }
    class MpcPads {
        constructor(hit, status) { this.hit = hit; this.status = status; this.enabled = false; this.generation = 0; }
        start() {
            if (this.enabled) return;
            this.enabled = true; this.last = null; this.generation++;
            this.status('Connecting MPC pads…'); this.poll(this.generation);
        }
        stop() {
            this.enabled = false; this.generation++; this.last = null;
            clearTimeout(this.timer); this.request?.abort();
        }
        async poll(generation) {
            if (!this.enabled || generation !== this.generation) return;
            if (document.hidden) this.last = null;
            else {
                const request = new AbortController(); this.request = request;
                const timeout = setTimeout(() => request.abort(), 2000);
                try {
                    const response = await fetch('/surface', {cache: 'no-store', signal: request.signal});
                    if (!response.ok) throw Error('MPC disconnected');
                    const state = await response.json();
                    if (!Array.isArray(state.pad_hits) || state.pad_hits.length !== 16) throw Error('MPC pad preview requires Tape Workshop v95 or later');
                    if (!this.enabled || generation !== this.generation) return;
                    if (document.hidden) { this.last = null; } else {
                    const counts = state.pad_hits;
                    if (this.last) counts.forEach((count, i) => {
                        // A counter distinguishes quick released taps and repeated hits.
                        // Never replay a backlog or a reset counter as a burst of notes.
                        if (Number.isInteger(count) && count > this.last[i]) this.hit(i);
                    });
                    this.last = counts.slice();
                    this.status('MPC pads enabled. Preview audio plays on this phone or computer.');
                    }
                } catch (error) {
                    if (this.enabled && generation === this.generation) {
                        this.last = null;
                        this.status(error.name === 'AbortError' ? 'Waiting for the MPC…' : error.message);
                    }
                } finally { clearTimeout(timeout); }
            }
            if (this.enabled && generation === this.generation) this.timer = setTimeout(() => this.poll(generation), 60);
        }
    }
    window.WorkshopPadPlayer = PadPlayer;
    window.WorkshopMpcPads = MpcPads;
})();
