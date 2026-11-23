# Tape Workshop v128

Based on Melodee by Kerem Kiliç (Ellic Studio).

Product-photo CSS now explicitly shows the A–H bank and stereo meter overlay. This was previously hidden by the general photo-overlay rules. Bank labels were visually verified on the local browser preview. Audio meters require Enable MPC audio; the meter timer also restarts after returning from browser history.

Screen fetches now run single-flight with a request-local abort timeout, avoiding stale requests aborting newer requests. Brief audio fetch failures are retried before ending the session. Returning to the page restores audio if it was previously enabled; manually stopping audio does not enable auto-resume. Recording still stops if the page is hidden to avoid saving an unreliable take.

The connected unit was running v127 during inspection. Its active recording was left untouched. Hardware testing of the new recovery behavior remains pending.
