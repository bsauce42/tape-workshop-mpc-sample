# Tape Workshop release updates

Tape Workshop extends Melodee by Kerem Kiliç (Ellic Studio). The original SD
installer still verifies Ed25519 signatures, checks every installed file,
self-tests the new application and restores the previous version on failure.
The key in `src/melodee/shell/update.h` now identifies Tape Workshop releases.

Install the first build with this key (v79) manually by copying its autorun
folder to the card. Earlier Tape Workshop builds still carry Melodee's key;
they cannot accept an update signed with Tape Workshop's new key. Do not
remove signature verification to bypass this transition.

## Publish a release

1. Keep the private signing key at `.local/tape-workshop-release-signing.pem`
   private and backed up. It is ignored by Git and never shipped. Keep using
   this same key for subsequent releases.
2. Build and stage the new version, for example
   `v0.3.0-tapeworkshop-community.80`, including `autorun/melodee/VERSION` and
   refreshed `SHA256SUMS`. Install `cryptography` in the build Python environment.
3. Run `python3 tools/workshop_release.py --package STAGED_PACKAGE --output
   dist/Tape-Workshop-v80-Signed-Update.zip`. Run the same command with
   `--verify ZIP` to verify every signed autorun file.
4. Publish a normal, non-draft, non-prerelease GitHub release in
   `bsauce42/tape-workshop-mpc-sample` with that exact full version as the tag.
   Attach the signed update ZIP, its SHA-256 file and the corresponding source
   archive. The source archive can remain a separate download.
5. To enable automatic browser-to-MPC transfer, also put the exact signed ZIP
   at the root of the repository's `updates` branch, retaining its file name.
   This is the browser-readable mirror at raw.githubusercontent.com. GitHub
   release asset downloads can reject cross-origin browser requests; the page
   tries the release API, then this mirror. The ZIP is below GitHub's 100 MiB
   single-file limit. Do not commit signing keys or credentials to any branch.

The local companion checks GitHub when it connects, caches successful checks
for ten minutes and has a Check now button. Internet access comes from the
phone or computer. Users can enable automatic download, download the ZIP to
Files instead, or choose a local signed ZIP manually. Installation can be requested from the webpage or confirmed on the MPC.
Optional automatic installation waits until playback and recording stop.
All paths use the existing project-save and recovery workflow. Unsaved,
unnamed projects must be saved before remote installation. Queued requests
can be cancelled from the page and are cleared when an offer changes.
Updating these local files does not publish a release to GitHub by itself.
