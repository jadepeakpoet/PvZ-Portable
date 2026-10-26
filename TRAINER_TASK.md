# Toolkit implementation record

Scope: compile-in toolkit for PvZ-Portable, desktop second window, mobile in-game
panel, immediate Chinese/English switching. Baseline 396f1c7 (0.2.6).
Branch: feat/trainer-toolkit. Existing worktree was clean; build/ contains local
resources, saves and launcher and remains ignored.

Architecture: Trainer/ owns service, feature catalog, UI, font and desktop host.
Game accesses only Trainer flags at narrow rule hooks. UI submits generation-bound
commands executed on main thread. PVZ_TRAINER independently enables the module.

Reference: locally inspected pvztoolkit UI and pvz.h;
https://github.com/lmintlcx/pvztoolkit (GPL-3.0). Reimplement behavior against
portable source; do not transplant Windows memory patches or FLTK code.

Ownership: coordinator owns Trainer.h, UI/font/platform, LawnApp, framework input,
CMake and commits. Service agent owns TrainerService.cpp and formats. Rule agent
owns narrow hooks in Lawn and framework persistence (excluding LawnApp and input).

Required verification: focused command/lifecycle and formats tests, trainer-on and
trainer-off builds, macOS real second window/event isolation/language visual QA,
mobile compile/UI where tools permit. Report unperformed platform checks honestly.

Milestone 1: core service, translated catalog, version-bound commands, narrow game
rule hooks, compatible lineup codecs and PAK tools implemented. Format and catalog
tests pass; first real-engine integration pass: 45 checks, zero failures, isolated
build/trainer-tests/run-gqn4h6w7. Independent review found/fixed queued-capacity,
imitater roundtrip/sleep, unsafe native spawn selection and partial YAML apply.

Milestone 1 committed as 08be4c1.

Milestone 2 complete: desktop second window, mobile in-game panel, live/persisted
zh/en choice, native-game controls, multilingual names/type pickers, continuous
placement, multiline editor, screenshots, platform file selection and ZIP export.
Unicode font discovery checks all catalog/name glyphs and TTC faces; CoreText
locates Apple system fonts. No OS fonts or game resources tracked.

Validation:
- Release with PVZ_TRAINER=ON, PVZ_DEBUG=OFF: build success.
- Release with PVZ_TRAINER=OFF: build/no-trainer success.
- CTest: trainer-formats, trainer-catalog, trainer-files, trainer-zip: all pass.
- Final real-engine desktop: 77 checks / 0 failures, run-scbndkke.
- Final forced-mobile layout: 75 checks / 0 failures, run-aoq9j1qd.
  Artifacts under build/trainer-tests; isolated saves for every run.
- Actual zh/en screenshots visually checked; missing simplified glyphs fixed.
  Screenshot command verified against decoded nonuniform game-window PNG.
- iOS ARM64 SDK syntax compilation: file bridge and CoreText font pass.
- Android helper javac with API36 + DocumentFile: pass (stage agent evidence).
- Files async lifecycle and ZIP tested with ASan/UBSan; malformed lineup fuzz,
  compatibility fixture and PAK traversal cases checked independently.
- Independent review fixes: restore GL context after desktop surface draw; clear
  held game input across window focus; hide/block toolkit during demo; reject old
  board commands; use local output staging on mobile, not external-folder import.

Limits: no Windows/Linux runtime or Android/iOS device execution; native file
picker human interaction not tested. Large directory ZIP export is bounded but
synchronous, so responsiveness for large archives is not performance-accepted.
User requested feat/trainer-toolkit; origin/main was fetched and matches baseline
396f1c7. Only new task commits will be pushed to the same-named feature branch.
Usage and maintenance are documented in docs/TRAINER.md.
