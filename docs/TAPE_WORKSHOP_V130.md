# Tape Workshop v130

Based on Melodee by Kerem Kiliç (Ellic Studio).

Live/Tutorial and other non-expansion pages skip the three expansion-builder scripts. The single-threaded HTTP server accepts a larger pending queue and bounds idle request headers to two seconds, while preserving fifteen-second upload-body and download-send timeouts.

Status requests are single-flight, bounded to three seconds and initially show Connecting rather than an immediate disconnected message. A successful screen frame can confirm connection independently of the status endpoint. Actual startup timing still needs a connected-device test after installation.
