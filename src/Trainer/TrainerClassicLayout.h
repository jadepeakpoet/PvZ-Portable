// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "Trainer.h"
#include <vector>
namespace Trainer::Classic {
enum class Kind { Button, Check, Input, Choice, Label, Text };
enum Id {
  SunInput = 1000,
  MoneyInput,
  TreeInput,
  Mode,
  Adventure,
  Rounds,
  SetSun = 1100,
  SetMoney,
  SetTree,
  Mix,
  SetRounds,
  Unlock,
  Win,
  Row = 1200,
  Col,
  Plant,
  Zombie,
  Mowers,
  ClearType,
  PutPlant = 1210,
  PutZombie,
  Grave,
  Ladder,
  Rake,
  Clear,
  Imitater = 1220,
  BrightestCob,
  Slot = 1300,
  Seed,
  Scene,
  LineupFile,
  LineupText,
  SetSlot = 1310,
  Quick,
  AutoLadder,
  Lily,
  Pot,
  ResetScene,
  LoadLineup,
  GetLineup,
  CopyLineup,
  PasteLineup,
  ApplyLineup,
  Screenshot,
  SeedImitater,
  SpawnFirst = 1400,
  GigaWeight = 1430,
  GigaLimit,
  SpawnMode,
  Generate,
  Details,
  Music = 1500,
  Debug,
  Speed,
  File,
  Dir,
  PlayMusic = 1510,
  SaveFolder,
  ChooseFile,
  Unpack,
  ChooseDir,
  Pack,
  SetDebug,
  SetSpeed,
  Documentation,
  About,
  Tooltips,
  Language = 201,
  Close = 202,
  FlagFirst = 2000
};
struct Control {
  int id, page;
  Kind kind;
  int x, y, w, h;
  const char *label;
};
std::vector<Control> Layout(bool chinese);
inline int Width(bool chinese) { return chinese ? 450 : 536; }
inline int Height(bool chinese) { return chinese ? 275 : 280; }
} // namespace Trainer::Classic
