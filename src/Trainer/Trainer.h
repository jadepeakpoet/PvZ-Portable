// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

class LawnApp;
namespace Trainer {
enum class Flag {
 SunLimit, AutoCollect, NoLoot, Fertilizer, BugSpray, Chocolate, TreeFood,
 FreePlanting, Anywhere, FastBelt, LockShovel, PlantInvincible, PlantWeak,
 ZombieInvincible, ZombieWeak, Reload, Butter, StopSpawning, StopZombies,
 Awake, NoCrater, NoIce, NoExplode, NoFog, SeeVase, Background, Readonly,
 Limbo, NoCooldown, Count
};
enum class Action {
 Sun, Money, TreeHeight, Unlock, Win, MixMode, Endless, Plant, Zombie, Grave,
 Ladder, Rake, Mowers, ClearPlants, ClearZombies, ClearItems, Slot, AutoLadder,
 Lily, Pot, Scene, ExportLineup, ImportLineup, SaveLineup, LoadLineup, Screenshot,
 SpawnGenerate, SpawnRead, SpawnApply, SpawnSave, SpawnLoad, Music, SaveFolder,
 Pack, Unpack, Debug, Speed, Reset, Documentation, About
};
struct Feature {
 int page; const char* en; const char* zh; const char* hintEn; const char* hintZh;
 int flag = -1; Action action = Action::Reset;
};
struct Command {
 Action action; int value = 0; int row = 0; int col = 0; int extra = 0;
 std::string text; std::uint64_t generation = 0;
};
struct Snapshot {
 bool board = false, profile = false, replay = false;
 int sun = 0, coins = 0, mode = 0, wave = 0, rows = 0, scene = 0;
 std::uint64_t generation = 0;
};
const std::vector<Feature>& Features();
#ifdef PVZ_TRAINER
bool Enabled(Flag flag);
bool SuppressWrites();
class Service {
public:
 explicit Service(LawnApp* app);
 ~Service();
 void Tick();
 void InvalidateBoard();
 void Submit(Command command);
 void Set(Flag flag, bool enabled);
 bool Get(Flag flag) const;
 void AllOff();
 void SaveSettings();
 Snapshot state;
 std::string messageEn, messageZh, text;
 bool chinese = true;
 bool paused = false;
 double speed = 1.0;
 bool panelOpen = false;
 bool screenshotRequested = false;
 bool mobilePause = false;
 LawnApp* app;
private:
 void Execute(const Command& command);
 std::vector<Command> commands;
 std::array<bool, static_cast<int>(Flag::Count)> flags{};
 const void* boardIdentity = nullptr;
 std::uint64_t boardSession = 0;
};
#else
inline bool Enabled(Flag) { return false; }
inline bool SuppressWrites() { return false; }
#endif
}
