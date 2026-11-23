# Tape Workshop on-device performance checks

The read-only `/performance` endpoint returns existing service-thread readings:
per-core busy percentages (half-second intervals), recent peak audio render
budget use, per-track render shares, aggregate plug-in processing load, audio
underrun/capture counters and overload-guard interventions since startup.
Render percentage is audio deadline use, not whole-machine CPU utilization.
Plug-in processing shares can sum above 100% when work spans cores.
No new timers, locks, allocations or file reads run in the audio callback.

Collect baseline and 60-second phases: stopped, one instrument playing chords,
several instruments playing, effects added, then live UI and browser audio.
Record counter deltas, peak/p95 render use, per-core use and the exact project.
Compare the same playing project on live UI versus a static page. Do not claim
an unloaded project proves instrument performance, or that zero underruns with
guard steals means no voices were sacrificed. Browser/network packet losses
are distinct from the MPC's ALSA underruns.

Melodee by Kerem Kiliç (Ellic Studio); Tape Workshop by bsauce42.
