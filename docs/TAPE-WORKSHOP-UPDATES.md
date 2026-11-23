# Tape Workshop signed updates

Tape Workshop uses its own Ed25519 release key. The private key stays local and must never ship in releases. Only the corresponding public key is embedded in the OS and installer. Updates must also carry the `vX.Y.Z-tapeworkshop-community.N` release identity and the expected runtime application package. The internal runtime registry name remains `melodee`; the release identity is Tape Workshop.

The service-thread installer still verifies hashes, runs a RAM self-test, stages a replacement and restores the previous app on startup failure. No audio-engine settings change.

For boot-menu installations, the SD worker records the canonical launching autorun folder in its RAM release. The updater uses that folder only if it resolves inside the same SD card, either the root autorun or an app's nested autorun. The original startup ownership check remains. The main boot menu and other app folders are never replaced by a nested update. Existing v88 installations need one direct SD-card upgrade because their RAM worker did not record the nested origin.

This restricts Tape Workshop's update channel, not the MPC boot menu: other operating systems remain usable. Derived from Melodee by Kerem Kiliç (Ellic Studio).

Validation: generated shell syntax, ARM64 build and sandboxed install/rollback tests. On this Mac, the shell tests use real Ed25519 cryptography via a Python command shim because the installed LibreSSL lacks the needed OpenSSL 3 command; they do not exercise the MPC's installed OpenSSL binary. Physical restart and update confirmation remain necessary.
