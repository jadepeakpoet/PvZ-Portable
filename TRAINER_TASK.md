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

Milestone 3: user requested original PvZ Toolkit geometry instead of the custom
GUI. Implemented original five-page layouts, 450×275 zh / 536×280 en clients,
named per-field menus, original defaults and direct actions, twenty spawning
checkboxes, separate spawning-count table and mobile overlay. Language selection
is in Utilities. Geometry reference: local upstream window.cpp and README GIFs;
no FLTK or memory-patch implementation copied. Layout metadata has focused tests.
First Classic integration: desktop 86/0, mobile 82/0, artifacts run-byzmaaeg and
run-ac87uj6b. Reviewed real rendered zh/en screenshots against reference frames.
Final interaction fixes cover popup lifetime/modal dismissal, Escape, right-click
spawn menu, and auxiliary-window replay/visibility behavior. Device/runtime limits
from milestone 2 still apply.

Final Classic verification after interaction fixes: Release build passed;
CTest 5/5 passed; real-engine desktop 86/0 and forced-mobile 82/0 passed.
Graphical tests required the macOS graphical session (sandbox-only launches could
not create an OpenGL context). No device tests claimed.

Independent final UI review found and resolved desktop resize being mistaken for
mobile embedding, and missing mobile gesture access. Embedding is now explicit;
mobile Generate supports long-press algorithm selection, and statistics supports
drag scrolling and long-press context menus. Added long-press integration coverage
(mobile now 83 checks). Reference geometry and core tests remain unchanged.

2026-10-27 simulator package: user requested iOS Simulator rather than real-device
IPA. Added opt-in PVZ_DEFAULT_UNLOCKED for new profiles, shared monotonic content
unlock routine, and iOS bundle resource fallback with Documents precedence.
Simulator Release ARM64 build passed, CTest 5/5, desktop engine integration 88/0
including unlock persistence and preserving existing coins/records. Package
contains local resources and install.command under ignored build-ios-simulator;
no game assets or machine-local dependency overlays are committed. Existing
profiles are not automatically overwritten. Not a device IPA or device test.
