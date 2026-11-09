# Maelstrom personal remaster

This is a modified edition of the SDL Maelstrom port, not a new Ambrosia
release. See COPYING, CREDITS and Data/PersonalArt/README.md for attribution.

## Build and run

Initialize the pinned upstream submodules (`git submodule update --init --recursive`).
Use the upstream CMake build instructions. For a standalone non-Steam Windows
build: `cmake -S . -B build -DSTEAM=OFF`, then
`cmake --build build --config Release`. Install/package with the upstream
CMake targets so Data and required SDL libraries accompany the executable.
No proprietary SDK or live Silas installation is required.

Launch with `--modern-art --source-sprites` for the remaster or
`--classic-art` for the original presentation. No flags retains upstream-style
presentation apart from pre-existing optional image filtering. Classic wins
when conflicting flags are supplied. Do not delete original Data assets.

The upstream Emscripten CMake target also builds. Its generated HTML needs
Module.arguments set to ['--modern-art','--source-sprites'] before the generated
script loads to select this presentation (or ['--classic-art'] for classic).
Serve its HTML/JS/WASM/data together over HTTP. Browser audio waits for a
user gesture; browser multiplayer remains unavailable in the upstream network
stub. Browser storage persistence has not been regression-certified.

## Scope

- Clean opt-in menu, controls, guide and credits, with original complete credits
  and story still reachable. Abel typography and original bitmap fallback.
- 328 HD source-art display frames, 245 procedural display-effect frames,
  procedural sphere shield and high-resolution HUD/multiplier/points icons.
- Original collision surfaces, sprite CRC, frame timing and game rules retained.
- Original assets unchanged. Classic mode provides runtime rollback.
- All images/fonts needed for these paths are in this repository; no downloads
  or generated-art service is used at runtime.

## Verification checkpoint

Windows Release and Emscripten builds passed. Practical native modern/classic
checks covered launching, movement/fire/scoring, shield rendering, controls
Cancel and saved bindings across relaunch, credits and original credits.
Browser modern gameplay, deaths/scoring, menu return and pause were observed;
classic browser gameplay was also checked. Original sprite CRC stayed 052a49a8.
These are targeted checks, not exhaustive certification. Live gameplay testing
was stopped at the owner's request and was not resumed for publication.

Three comparable Windows launch-to-unfaded-menu measurements were
3.764 / 3.753 / 3.883 seconds (mean 3.800), versus prior 4.611 / 4.411 / 4.379.
The timer includes process launch and a 20ms log poll, not physical scan-out.
This is a warm local-machine measurement, not a cold-start/browser guarantee.

Remaining limits: multiplayer and high-score persistence are not certified;
native automated name entry was inconclusive. Late-wave and every-sprite
coverage is incomplete. Steel asteroids, damaged ships and multiplayer art
remain original. Source sprites use planar rotation within original frame
bounds, not newly authored tumbling animations; silhouette and collision mask
are intentionally decoupled. No claim that all artwork is HD.

## Non-interactive checks

`git diff --check` and `python tests/personal_art_static.py` check whitespace,
XML readability, bundled art/font signatures and asset references without
launching the game. CI build results must be checked separately.
