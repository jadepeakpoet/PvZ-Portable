# Toolkit implementation record

Scope: compile-in toolkit for PvZ-Portable, desktop second window, mobile in-game
panel, immediate Chinese/English switching. Baseline 396f1c7 (0.2.6).
Branch: feat/trainer-toolkit. Existing worktree was clean; build/ contains local
resources, saves and launcher and remains ignored.

Architecture: Trainer/ owns service, feature catalog, UI, font and desktop host.
Game accesses only Trainer flags at narrow rule hooks. UI submits generation-bound
commands executed on main thread. PVZ_TRAINER independently enables the module.

Reference: local /Users/wuyufeng/coding/github/pvztoolkit, UI and pvz.h;
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

Status: desktop/mobile GUI integration and platform file dialogs in progress.
Next: final GUI/focus/placement tests, compile switch-off build, platform syntax
checks, final milestone review. No mobile-device or Windows/Linux runtime pass
claimed. User requested branch feat/trainer-toolkit and immediate zh/en switch.
