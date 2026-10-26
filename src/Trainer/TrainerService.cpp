// SPDX-License-Identifier: LGPL-3.0-or-later
#include "Trainer.h"
#ifdef PVZ_TRAINER
#include "Lawn/Board.h"
#include "Lawn/Challenge.h"
#include "Lawn/Cutscene.h"
#include "Lawn/GridItem.h"
#include "Lawn/LawnMower.h"
#include "Lawn/Plant.h"
#include "Lawn/Projectile.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/System/Music.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/Widget/ChallengeScreen.h"
#include "Lawn/Widget/GameSelector.h"
#include "Lawn/Zombie.h"
#include "LawnApp.h"
#include "PvzpLib/Reanimator.h"
#include "SexyAppFramework/Common.h"
#include "SexyAppFramework/graphics/GLInterface.h"
#include "SexyAppFramework/graphics/Image.h"
#include "TrainerFiles.h"
#include "TrainerFormats.h"
#include "widget/WidgetManager.h"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>

namespace Trainer {
namespace {
Service *active = nullptr;
std::string ErrorChinese(const std::string &error) {
  static const std::pair<const char *, const char *> messages[] = {
      {"This operation requires a normal lawn level",
       "该操作仅适用于普通战场关卡"},
      {"Native spawning requires normal zombie ID 0",
       "原生出怪需要选择普通僵尸 ID 0"},
      {"Water zombie requires a pool row", "水路僵尸必须位于泳池水路"},
      {"Zomboni cannot spawn on the roof", "屋顶不能生成冰车"},
      {"Bobsled requires an existing ice lane", "雪橇队需要已有冰道"},
      {"Cannot read file", "无法读取文件"},
      {"Text file exceeds 1 MB", "文本文件超过 1 MB"},
      {"Destination already exists", "目标已存在，请选择新路径"},
      {"Cannot write file", "无法写入文件"},
      {"Row or column outside current board", "行或列超出当前战场范围"},
      {"Cannot place rake in pool", "水路不能放钉耙"},
      {"A rake already exists", "战场上已有钉耙"},
      {"Grid item capacity reached", "场地物品数量已达上限"},
      {"Scene must be 0..5", "场地类型必须为 0..5"},
      {"Lineup is only available on lawn scenes", "只有普通战场支持阵型操作"},
      {"Invalid plant position", "植物位置无效"},
      {"Board capacity too small for lineup", "当前战场容量不足以加载阵型"},
      {"One nonempty line is required for each wave",
       "出怪列表必须每波一行且不能为空"},
      {"Boss cannot appear in a wave list", "普通出怪列表不能包含僵王"},
      {"Enter a level first", "请先进入关卡"},
      {"Sun must be 0..2000000000", "阳光数必须为 0..2000000000"},
      {"Select a profile; coins must be 0..2000000000",
       "请先选择用户，金币数必须为 0..2000000000"},
      {"Tree height must be 0..1000000", "智慧树高度必须为 0..1000000"},
      {"Select a profile first", "请先选择用户"},
      {"Wait until gameplay starts", "请等待关卡正式开始"},
      {"Invalid mode ID or no profile", "模式 ID 无效或尚未选择用户"},
      {"Adventure level must be 1..50", "冒险关卡必须为 1..50"},
      {"Enter survival; stage must be 0..1000000",
       "请进入生存模式，阶段必须为 0..1000000"},
      {"Plant ID must be 0..47", "植物 ID 必须为 0..47"},
      {"Plant capacity reached", "植物数量已达上限"},
      {"Zombie ID must be 0..32", "僵尸 ID 必须为 0..32"},
      {"Zombie capacity reached", "僵尸数量已达上限"},
      {"Boss requires its own level", "僵王只能在专属关卡生成"},
      {"Cannot place grave here", "当前位置不能放置墓碑"},
      {"Ladder already exists", "当前位置已有梯子"},
      {"Mower action must be 0..2", "小推车操作必须为 0..2"},
      {"Mower capacity reached", "小推车数量已达上限"},
      {"Invalid grid item type", "场地物品类型无效"},
      {"Invalid seed slot or plant ID", "卡槽或植物 ID 无效"},
      {"Current terrain does not match", "当前场地不支持该操作"},
      {"Lineup entry must be nonnegative", "阵型条目索引不能为负数"},
      {"No valid Toolkit lineup code at selected index",
       "所选条目没有有效的 Toolkit 阵型码"},
      {"Select at least one zombie ID", "请至少选择一种僵尸"},
      {"Spawn mode must be 0..2", "出怪模式必须为 0..2"},
      {"Enemy count must be 1..50", "每波僵尸数量必须为 1..50"},
      {"Red-eye limit must be 0 or 1", "红眼变速选项必须为 0 或 1"},
      {"Red-eye weight must be 1000..6000", "红眼权重必须为 1000..6000"},
      {"Select a regular zombie for all waves",
       "请至少选择一种可用于所有波次的普通僵尸"},
      {"Music must be 1..13", "音乐编号必须为 1..13"},
      {"Cannot open documentation on this platform",
       "当前平台无法打开文档网站"},
      {"Cannot open save directory on this platform",
       "当前平台无法打开存档目录"},
      {"Enter source path, newline, destination path",
       "请填写源路径、换行、目标路径"},
      {"Debug mode must be 0..4", "调试模式必须为 0..4"},
      {"Speed must be 0 or 10..1000 percent",
       "速度必须为 0 或 10..1000 百分比"},
      {"Invalid lineup code length", "阵型码长度无效"},
      {"Invalid base64 padding", "阵型码填充字符无效"},
      {"Invalid lineup code alphabet", "阵型码包含非法字符"},
      {"Invalid lineup scene/rake row", "阵型场地或钉耙行无效"},
      {"Rake cannot be in pool", "钉耙不能位于水路"},
      {"Invalid lineup plant ID", "阵型植物 ID 无效"},
      {"Truncated PAK", "PAK 文件不完整"},
      {"Lineup compression failed", "阵型压缩失败"},
      {"Truncated lineup", "阵型码不完整"},
      {"Invalid lineup compressed data", "阵型压缩数据无效"},
      {"Invalid integer list", "整数列表格式或范围无效"},
      {"PAK source must be directory", "PAK 打包源必须为文件夹"},
      {"Symlinks are not supported", "不支持符号链接"},
      {"PAK exceeds size limits", "PAK 超过大小限制"},
      {"Cannot write PAK", "无法写入 PAK 文件"},
      {"Unsupported PAK header", "不支持的 PAK 文件头"},
      {"Truncated PAK path", "PAK 文件路径不完整"},
      {"Unsafe PAK path", "PAK 包含不安全的路径"},
      {"Too many PAK entries", "PAK 文件条目过多"},
      {"Invalid PAK data length", "PAK 数据长度无效"},
      {"Duplicate PAK path", "PAK 包含重复路径"},
      {"Cannot extract PAK file", "无法解压 PAK 文件"},
  };
  for (const auto &[en, zh] : messages)
    if (error == en)
      return zh;
  return "操作失败：文件格式或参数无效";
}
std::filesystem::path SettingsPath() {
  return std::filesystem::u8path(
      Sexy::GetAppDataPath("trainer/settings-v1.txt"));
}
std::string ReadText(const std::string &path) {
  std::ifstream in(std::filesystem::u8path(path), std::ios::binary);
  if (!in)
    throw std::runtime_error("Cannot read file");
  std::string result{std::istreambuf_iterator<char>(in), {}};
  if (result.size() > 1024 * 1024)
    throw std::runtime_error("Text file exceeds 1 MB");
  return result;
}
void WriteText(const std::string &path, const std::string &text) {
  auto p = std::filesystem::u8path(path);
  if (std::filesystem::exists(p))
    throw std::runtime_error("Destination already exists");
  std::ofstream out(p, std::ios::binary);
  out << text;
  if (!out)
    throw std::runtime_error("Cannot write file");
}
void Require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
int Rows(Board *b) { return b && (b->StageHasPool()) ? 6 : 5; }
void Cell(Board *b, int row, int col) {
  Require(b && row >= 0 && row < Rows(b) && col >= 0 && col < 9,
          "Row or column outside current board");
}
void ValidateZombieTerrain(Board *b, int type, int row = -1) {
  if (type == int(ZombieType::ZOMBIE_SNORKEL) ||
      type == int(ZombieType::ZOMBIE_DOLPHIN_RIDER) ||
      type == int(ZombieType::ZOMBIE_DUCKY_TUBE))
    Require(b->StageHasPool() && (row < 0 || b->IsPoolSquare(0, row)),
            "Water zombie requires a pool row");
  if (type == int(ZombieType::ZOMBIE_ZAMBONI))
    Require(!b->StageHasRoof(), "Zomboni cannot spawn on the roof");
  if (type == int(ZombieType::ZOMBIE_BOBSLED)) {
    bool ice = false;
    for (int r = 0; r < Rows(b); ++r)
      if ((row < 0 || r == row) && b->mIceMinX[r] < 800 &&
          b->mPlantRow[r] != PlantRowType::PLANTROW_POOL)
        ice = true;
    Require(ice, "Bobsled requires an existing ice lane");
  }
}
void PutRake(Board *b, int row, int col) {
  Cell(b, row, col);
  Require(b->mPlantRow[row] != PlantRowType::PLANTROW_POOL,
          "Cannot place rake in pool");
  Require(!b->GetRake(), "A rake already exists");
  Require(b->mGridItems.mSize < b->mGridItems.mMaxSize,
          "Grid item capacity reached");
  auto *r = b->mGridItems.DataArrayAlloc();
  r->mGridItemType = GridItemType::GRIDITEM_RAKE;
  r->mGridX = col;
  r->mGridY = row;
  r->mPosX = b->GridToPixelX(col, row);
  r->mPosY = b->GridToPixelY(col, row);
  r->mRenderOrder =
      b->MakeRenderOrder(RenderLayer::RENDER_LAYER_GRAVE_STONE, row, 9);
  r->mGridItemReanimID =
      b->mApp->ReanimationGetID(b->CreateRakeReanim(r->mPosX, r->mPosY, 0));
  r->mGridItemState = GridItemState::GRIDITEM_STATE_RAKE_ATTRACTING;
}
void ClearPlants(Board *b) {
  for (auto *p : b->mPlants)
    if (!p->mDead)
      p->Die();
}
void ClearItems(Board *b, int type) {
  for (auto *g : b->mGridItems)
    if (!g->mDead && (type < 0 || int(g->mGridItemType) == type))
      g->GridItemDie();
}
void SetScene(Board *b, int scene) {
  Require(scene >= 0 && scene <= 5, "Scene must be 0..5");
  // Remove objects before reducing rows; their animations are disposed by game
  // methods.
  ClearPlants(b);
  b->RemoveAllZombies();
  ClearItems(b, -1);
  for (auto *m : b->mLawnMowers)
    if (!m->mDead)
      m->Die();
  b->ProcessDeleteQueue();
  b->mBackground = static_cast<BackgroundType>(scene);
  b->LoadBackgroundImages();
  std::sort(b->mLoadedResourceNames.begin(), b->mLoadedResourceNames.end());
  b->mLoadedResourceNames.erase(std::unique(b->mLoadedResourceNames.begin(),
                                            b->mLoadedResourceNames.end()),
                                b->mLoadedResourceNames.end());
  for (auto *projectile : b->mProjectiles)
    if (!projectile->mDead)
      projectile->Die();
  for (int y = 0; y < 6; ++y) {
    b->mIceMinX[y] = BOARD_ICE_START;
    b->mIceTimer[y] = 0;
  }

  bool pool = scene == 2 || scene == 3;
  for (int y = 0; y < 6; ++y) {
    b->mPlantRow[y] = (pool && (y == 2 || y == 3)) ? PlantRowType::PLANTROW_POOL
                      : (!pool && y == 5)          ? PlantRowType::PLANTROW_DIRT
                                          : PlantRowType::PLANTROW_NORMAL;
    for (int x = 0; x < 9; ++x)
      b->mGridSquareType[x][y] =
          (pool && (y == 2 || y == 3)) ? GridSquareType::GRIDSQUARE_POOL
          : (!pool && y == 5)          ? GridSquareType::GRIDSQUARE_DIRT
                                       : GridSquareType::GRIDSQUARE_GRASS;
  }
  b->InitLawnMowers();
  b->mSodPosition = 800;
  b->mFogOffset = 0;
  b->mFogBlownCountDown = 0;
  b->MarkAllDirty();
}
Formats::Lineup Capture(Board *b) {
  Formats::Lineup l;
  l.scene = int(b->mBackground);
  Require(l.scene <= 5, "Lineup is only available on lawn scenes");
  for (auto *p : b->mPlants) {
    if (p->mDead)
      continue;
    int i = p->mRow * 9 + p->mPlantCol;
    Require(i >= 0 && i < 54, "Invalid plant position");
    auto &cell = l.cells[i];
    bool im = p->mImitaterType != SeedType::SEED_NONE;
    int type = int(p->mSeedType == SeedType::SEED_IMITATER ? p->mImitaterType
                                                           : p->mSeedType);
    if (type == int(SeedType::SEED_LILYPAD) ||
        type == int(SeedType::SEED_FLOWERPOT)) {
      cell = (cell & ~0xE0) |
             ((type == int(SeedType::SEED_LILYPAD) ? 1 : 2) << 6) |
             (im ? 32 : 0);
    } else if (type == int(SeedType::SEED_PUMPKINSHELL))
      cell |= 16 | (im ? 8 : 0);
    else if (type == int(SeedType::SEED_INSTANT_COFFEE))
      cell |= 4 | (im ? 2 : 0);
    else if (type >= 0 && type < 48)
      cell = (cell & 0xFF) | ((type + 1) << 10) | (im ? 512 : 0) |
             (!p->mIsAsleep ? 256 : 0);
  }
  for (auto *g : b->mGridItems) {
    if (g->mDead)
      continue;
    int i = g->mGridY * 9 + g->mGridX;
    if (i < 0 || i >= 54)
      continue;
    if (g->mGridItemType == GridItemType::GRIDITEM_LADDER)
      l.cells[i] |= 1;
    if (g->mGridItemType == GridItemType::GRIDITEM_GRAVESTONE)
      l.cells[i] = (l.cells[i] & ~192) | 192;
    if (g->mGridItemType == GridItemType::GRIDITEM_RAKE)
      l.rakeRow = g->mGridY + 1;
  }
  return l;
}
void ApplyLineup(Board *b, const Formats::Lineup &l) {
  Require(b->mPlants.mMaxSize >= 216 && b->mGridItems.mMaxSize >= 109,
          "Board capacity too small for lineup");
  SetScene(b, l.scene);
  for (int row = 0; row < Rows(b); ++row)
    for (int col = 0; col < 9; ++col) {
      auto v = l.cells[row * 9 + col];
      auto add = [&](int type, bool im) {
        auto seed = static_cast<SeedType>(type);
        auto *plant = b->AddPlant(
            col, row, seed, im ? SeedType::SEED_IMITATER : SeedType::SEED_NONE);
        if (im && plant) {
          bool less = seed == SeedType::SEED_HYPNOSHROOM ||
                      seed == SeedType::SEED_SQUASH ||
                      seed == SeedType::SEED_POTATOMINE ||
                      seed == SeedType::SEED_GARLIC ||
                      seed == SeedType::SEED_LILYPAD;
          for (auto id : {plant->mBodyReanimID, plant->mHeadReanimID,
                          plant->mHeadReanimID2, plant->mHeadReanimID3})
            if (auto *anim = b->mApp->ReanimationTryToGet(id))
              anim->mFilterEffect =
                  less ? FilterEffect::FILTER_EFFECT_LESS_WASHED_OUT
                       : FilterEffect::FILTER_EFFECT_WASHED_OUT;
        }
        return plant;
      };
      int base = (v >> 6) & 3;
      if (base == 1)
        add(int(SeedType::SEED_LILYPAD), v & 32);
      if (base == 2)
        add(int(SeedType::SEED_FLOWERPOT), v & 32);
      if (base == 3)
        b->AddAGraveStone(col, row);
      if (int type = (v >> 10) - 1; type >= 0) {
        auto *p = add(type, v & 512);
        if (p && Plant::IsNocturnal(static_cast<SeedType>(type)))
          p->SetSleeping(!(v & 256));
      }
      if (v & 16)
        add(int(SeedType::SEED_PUMPKINSHELL), v & 8);
      if (v & 4)
        add(int(SeedType::SEED_INSTANT_COFFEE), v & 2);
      if (v & 1)
        b->AddALadder(col, row);
    }
  if (l.rakeRow)
    PutRake(b, l.rakeRow - 1, 7);
}
std::string SpawnText(Board *b) {
  std::ostringstream out;
  for (int w = 0; w < b->mNumWaves; ++w) {
    for (int i = 0; i < MAX_ZOMBIES_IN_WAVE; ++i) {
      auto t = b->mZombiesInWave[w][i];
      if (t == ZombieType::ZOMBIE_INVALID)
        break;
      if (i)
        out << ' ';
      out << int(t);
    }
    out << '\n';
  }
  return out.str();
}
void ApplySpawn(Board *b, const std::string &text) {
  std::istringstream in(text);
  std::string line;
  std::vector<std::vector<int>> waves;
  while (std::getline(in, line)) {
    if (line.empty())
      continue;
    waves.push_back(Formats::ParseIntegers(line, 0, 32, MAX_ZOMBIES_IN_WAVE));
  }
  for (const auto &wave : waves)
    for (int t : wave) {
      Require(t != int(ZombieType::ZOMBIE_BOSS),
              "Boss cannot appear in a wave list");
      ValidateZombieTerrain(b, t);
    }
  Require(waves.size() == std::size_t(b->mNumWaves),
          "One nonempty line is required for each wave");
  for (std::size_t w = 0; w < waves.size(); ++w)
    for (int i = 0; i < MAX_ZOMBIES_IN_WAVE; ++i)
      b->mZombiesInWave[w][i] = i < int(waves[w].size())
                                    ? static_cast<ZombieType>(waves[w][i])
                                    : ZombieType::ZOMBIE_INVALID;
}
} // namespace

Service::Service(LawnApp *a) : app(a) {
  active = this;
  std::ifstream in(SettingsPath());
  std::string key;
  double n;
  while (in >> key >> n) {
    if (key == "language")
      chinese =
          n != 0; /* Gameplay switches intentionally default off each launch. */
  }
  messageEn = "Ready";
  messageZh = "就绪";
}
Service::~Service() {
  SaveSettings();
  if (active == this)
    active = nullptr;
}
bool Enabled(Flag f) {
  return active && active->app && !active->app->IsInDemoMode() &&
         active->Get(f);
}
bool SuppressWrites() { return Enabled(Flag::Readonly); }
bool Service::Get(Flag f) const {
  auto i = static_cast<int>(f);
  return i >= 0 && i < int(flags.size()) && flags[i];
}
void Service::SaveSettings() {
  try {
    auto p = SettingsPath();
    std::filesystem::create_directories(p.parent_path());
    std::ofstream out(p);
    out << "language " << chinese << '\n';
  } catch (...) {
  }
}
void Service::Set(Flag f, bool on) {
  if (app->IsInDemoMode()) {
    messageEn = "Trainer is disabled during demo recording/playback";
    messageZh = "录制或回放期间禁用修改器";
    return;
  }
  int i = int(f);
  if (i < 0 || i >= int(flags.size()))
    return;
  flags[i] = on;
  if (on) {
    if (f == Flag::PlantInvincible)
      flags[int(Flag::PlantWeak)] = false;
    if (f == Flag::PlantWeak)
      flags[int(Flag::PlantInvincible)] = false;
    if (f == Flag::ZombieInvincible)
      flags[int(Flag::ZombieWeak)] = false;
    if (f == Flag::ZombieWeak)
      flags[int(Flag::ZombieInvincible)] = false;
  }
  messageEn = on ? "Enabled" : "Disabled";
  messageZh = on ? "已开启" : "已关闭";
}
void Service::AllOff() {
  flags.fill(false);
  paused = false;
  speed = 1.0;
  messageEn = "All effects disabled";
  messageZh = "已关闭全部效果";
}
void Service::InvalidateBoard() {
  boardIdentity = nullptr;
  commands.clear();
  ++state.generation;
  state.board = false;
}
void Service::Submit(Command c) {
  if (commands.size() >= 256) {
    messageEn = "Command queue is full";
    messageZh = "操作队列已满";
    return;
  }
  if (c.generation == 0)
    c.generation = state.generation;
  commands.push_back(std::move(c));
}
void Service::Tick() {
  auto *b = app->mBoard;
  if (b != boardIdentity || (b && b->mBoardUpdateCounter < boardSession)) {
    boardIdentity = b;
    ++state.generation;
    commands.clear();
  }
  boardSession = b ? b->mBoardUpdateCounter : 0;
  state.board = b != nullptr;
  state.profile = app->mPlayerInfo != nullptr;
  state.replay = app->IsInDemoMode();
  state.mode = int(app->mGameMode);
  state.sun = b ? b->mSunMoney : 0;
  state.coins = app->mPlayerInfo ? app->mPlayerInfo->mCoins * 10 : 0;
  state.rows = Rows(b);
  state.wave = b ? b->mCurrentWave : 0;
  state.scene = b ? int(b->mBackground) : 0;
  auto pending = std::move(commands);
  commands.clear();
  for (const auto &c : pending) {
    if (c.generation != state.generation) {
      messageEn = "Discarded command for previous level";
      messageZh = "已丢弃旧关卡操作";
      continue;
    }
    if (state.replay) {
      messageEn = "Trainer is disabled during demo recording/playback";
      messageZh = "录制或回放期间禁用修改器";
      continue;
    }
    try {
      Execute(c);
    } catch (const std::exception &e) {
      messageEn = e.what();
      messageZh = ErrorChinese(e.what());
    }
  }
}

void Service::Execute(const Command &c) {
  auto *b = app->mBoard;
  auto *p = app->mPlayerInfo;
  bool needsBoard =
      c.action != Action::Money && c.action != Action::TreeHeight &&
      c.action != Action::Unlock && c.action != Action::SaveFolder &&
      c.action != Action::Pack && c.action != Action::Unpack &&
      c.action != Action::Reset && c.action != Action::Speed &&
      c.action != Action::MixMode && c.action != Action::Music &&
      c.action != Action::Documentation && c.action != Action::About &&
      c.action != Action::Screenshot;
  Require(!needsBoard || b, "Enter a level first");
  if (needsBoard && c.action != Action::Debug)
    Require(int(b->mBackground) <= 5 &&
                app->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN &&
                app->mGameMode != GameMode::GAMEMODE_TREE_OF_WISDOM &&
                app->mGameMode != GameMode::GAMEMODE_CHALLENGE_ZOMBIQUARIUM,
            "This operation requires a normal lawn level");
  messageEn = "Applied";
  messageZh = "已应用";
  switch (c.action) {
  case Action::Sun:
    Require(c.value >= 0 && c.value <= 2000000000, "Sun must be 0..2000000000");
    b->mSunMoney = c.value;
    break;
  case Action::Money:
    Require(p && c.value >= 0 && c.value <= 2000000000,
            "Select a profile; coins must be 0..2000000000");
    p->mCoins = c.value / 10;
    app->WriteCurrentUserConfig();
    break;
  case Action::TreeHeight:
    Require(p && c.value >= 0 && c.value <= 1000000,
            "Tree height must be 0..1000000");
    p->mChallengeRecords[int(GameMode::GAMEMODE_TREE_OF_WISDOM) - 1] = c.value;
    if (b && app->mGameMode == GameMode::GAMEMODE_TREE_OF_WISDOM)
      b->mChallenge->TreeOfWisdomInit();
    app->WriteCurrentUserConfig();
    break;
  case Action::Unlock:
    Require(p, "Select a profile first");
    p->mFinishedAdventure = std::max(p->mFinishedAdventure, 2U);
    p->mHasUnlockedMinigames = p->mHasUnlockedPuzzleMode =
        p->mHasUnlockedSurvivalMode = 1;
    for (int i = 0; i < 100; ++i)
      p->mChallengeRecords[i] = std::max(20U, p->mChallengeRecords[i]);
    for (int i = 0; i <= int(StoreItem::STORE_ITEM_PLANT_IMITATER); ++i)
      p->mPurchases[i] = 1;
    app->WriteCurrentUserConfig();
    if (app->mGameSelector)
      app->mGameSelector->SyncProfile(true);
    break;
  case Action::Win:
    Require(app->mGameScene == GameScenes::SCENE_PLAYING,
            "Wait until gameplay starts");
    b->RemoveAllZombies();
    app->mBoardResult = BoardResult::BOARDRESULT_CHEAT;
    if (c.extra && app->IsSurvivalMode())
      b->mMainCounter += (75 - ((b->mMainCounter + 500) % 75)) % 75;
    b->mCurrentWave = b->mNumWaves;
    b->FadeOutLevel();
    break;
  case Action::MixMode:
    Require(p && c.value >= 0 && c.value < NUM_CHALLENGE_MODES,
            "Invalid mode ID or no profile");
    if (c.value == 0) {
      Require(c.extra >= 1 && c.extra <= 50, "Adventure level must be 1..50");
      p->SetLevel(c.extra);
    }
    app->PreNewGame(static_cast<GameMode>(c.value), false);
    break;
  case Action::Endless:
    Require(b->mChallenge && app->IsSurvivalMode() && c.value >= 0 &&
                c.value <= 1000000,
            "Enter survival; stage must be 0..1000000");
    b->mChallenge->mSurvivalStage = c.value;
    break;
  case Action::Plant: {
    Cell(b, c.row, c.col);
    Require(c.value >= 0 && c.value < 48, "Plant ID must be 0..47");
    Require(c.value != int(SeedType::SEED_COBCANNON) || c.col < 8,
            "Row or column outside current board");
    Require(b->mPlants.mSize + 1 < b->mPlants.mMaxSize,
            "Plant capacity reached");
    auto type = static_cast<SeedType>(c.value);
    b->AddPlant(c.col, c.row, c.extra ? SeedType::SEED_IMITATER : type,
                c.extra ? type : SeedType::SEED_NONE);
    break;
  }
  case Action::Zombie: {
    Cell(b, c.row, c.col);
    Require(c.value >= 0 && c.value <= 32, "Zombie ID must be 0..32");
    Require(b->mZombies.mSize + 5 < b->mZombies.mMaxSize,
            "Zombie capacity reached");
    Require(c.value != int(ZombieType::ZOMBIE_BOSS),
            "Boss requires its own level");
    ValidateZombieTerrain(b, c.value, c.row);
    auto *z = b->AddZombieInRow(static_cast<ZombieType>(c.value), c.row,
                                Zombie::ZOMBIE_WAVE_DEBUG);
    if (z) {
      z->mPosX = float(b->GridToPixelX(c.col, c.row));
      z->mX = int(z->mPosX);
    }
    break;
  }
  case Action::Grave:
    Cell(b, c.row, c.col);
    Require(b->mGridItems.mSize < b->mGridItems.mMaxSize,
            "Grid item capacity reached");
    Require(b->CanAddGraveStoneAt(c.col, c.row), "Cannot place grave here");
    b->AddAGraveStone(c.col, c.row);
    break;
  case Action::Ladder:
    Cell(b, c.row, c.col);
    Require(b->mGridItems.mSize < b->mGridItems.mMaxSize,
            "Grid item capacity reached");
    Require(!b->GetLadderAt(c.col, c.row), "Ladder already exists");
    b->AddALadder(c.col, c.row);
    break;
  case Action::Rake:
    PutRake(b, c.row, c.col);
    break;
  case Action::Mowers:
    Require(c.value >= 0 && c.value <= 2, "Mower action must be 0..2");
    if (c.value == 2) {
      b->ProcessDeleteQueue();
      Require(b->mLawnMowers.mSize + Rows(b) <= b->mLawnMowers.mMaxSize,
              "Mower capacity reached");
      for (int row = 0; row < Rows(b); ++row)
        if (!b->FindLawnMowerInRow(row)) {
          auto *m = b->mLawnMowers.DataArrayAlloc();
          m->LawnMowerInitialize(row);
          m->mVisible = true;
        }
    } else
      for (auto *m : b->mLawnMowers)
        if (!m->mDead) {
          if (c.value == 0)
            m->StartMower();
          else
            m->Die();
        }
    break;
  case Action::ClearPlants:
    ClearPlants(b);
    break;
  case Action::ClearZombies:
    b->RemoveAllZombies();
    break;
  case Action::ClearItems:
    Require(c.value >= -1 && c.value <= 12, "Invalid grid item type");
    ClearItems(b, c.value);
    break;
  case Action::Slot:
    Require(b->mSeedBank && c.col >= 0 && c.col < b->mSeedBank->mNumPackets &&
                c.value >= 0 && c.value < 48,
            "Invalid seed slot or plant ID");
    b->mSeedBank->mSeedPackets[c.col].SetPacketType(
        c.extra ? SeedType::SEED_IMITATER : static_cast<SeedType>(c.value),
        c.extra ? static_cast<SeedType>(c.value) : SeedType::SEED_NONE);
    break;
  case Action::AutoLadder:
    Require(b->mGridItems.mSize + b->mPlants.mSize <= b->mGridItems.mMaxSize,
            "Grid item capacity reached");
    for (auto *plant : b->mPlants)
      if (!plant->mDead && plant->mSeedType == SeedType::SEED_PUMPKINSHELL &&
          !b->IsPoolSquare(plant->mPlantCol, plant->mRow) &&
          !b->GetLadderAt(plant->mPlantCol, plant->mRow))
        b->AddALadder(plant->mPlantCol, plant->mRow);
    break;
  case Action::Lily:
  case Action::Pot: {
    auto type = c.action == Action::Lily ? SeedType::SEED_LILYPAD
                                         : SeedType::SEED_FLOWERPOT;
    Require(c.action == Action::Lily ? b->StageHasPool() : b->StageHasRoof(),
            "Current terrain does not match");
    Require(b->mPlants.mSize + 54 <= b->mPlants.mMaxSize,
            "Plant capacity reached");
    Require(c.value == 0 || (c.value >= 1 && c.value <= 9 &&
                             c.extra >= c.value && c.extra <= 9),
            "Row or column outside current board");
    for (int y = 0; y < Rows(b); ++y)
      for (int x = c.value ? c.value - 1 : 0; x < (c.value ? c.extra : 9); ++x)
        if (b->CanPlantAt(x, y, type) == PlantingReason::PLANTING_OK)
          b->AddPlant(x, y, type);
    break;
  }
  case Action::Scene:
    SetScene(b, c.value);
    break;
  case Action::ExportLineup:
    text = Formats::EncodeLineup(Capture(b));
    break;
  case Action::ImportLineup:
    ApplyLineup(b, Formats::DecodeLineup(c.text));
    break;
  case Action::SaveLineup:
    WriteText(c.text, Formats::EncodeLineup(Capture(b)) + "\n");
    break;
  case Action::LoadLineup: {
    std::string data = ReadText(c.text);
    std::istringstream in(data);
    std::string line;
    int index = 0;
    Require(c.value >= 0, "Lineup entry must be nonnegative");
    std::optional<Formats::Lineup> selected;
    std::string selectedCode;
    while (std::getline(in, line)) {
      if (line.empty() || line[0] == '#')
        continue;
      auto pos = line.find(':');
      if (pos != std::string::npos)
        line = line.substr(pos + 1);
      auto first = line.find_first_not_of(" \t\r\"'");
      if (first == std::string::npos)
        continue;
      line.erase(0, first);
      auto end = line.find_last_not_of(" \t\r\"'");
      if (end != std::string::npos)
        line.resize(end + 1);
      std::optional<Formats::Lineup> parsed;
      try {
        parsed = Formats::DecodeLineup(line);
      } catch (const std::exception &) {
        continue;
      }
      if (index++ == c.value) {
        selected = *parsed;
        selectedCode = line;
        break;
      }
    }
    Require(selected.has_value(),
            "No valid Toolkit lineup code at selected index");
    ApplyLineup(b, *selected);
    text = selectedCode;
    break;
  }
  case Action::SpawnGenerate: {
    auto types = Formats::ParseIntegers(c.text, 0, 32, 33);
    Require(!types.empty(), "Select at least one zombie ID");
    Require(c.value >= 0 && c.value <= 2, "Spawn mode must be 0..2");
    for (int t : types) {
      Require(t != int(ZombieType::ZOMBIE_BOSS),
              "Boss cannot appear in a wave list");
      ValidateZombieTerrain(b, t);
    }
    if (c.value == 0) {
      Require(std::find(types.begin(), types.end(), 0) != types.end(),
              "Native spawning requires normal zombie ID 0");
      std::fill(std::begin(b->mZombieAllowed), std::end(b->mZombieAllowed),
                false);
      for (int t : types)
        b->mZombieAllowed[t] = true;
      b->PickZombieWaves();
    } else {
      Require(c.extra >= 1 && c.extra <= MAX_ZOMBIES_IN_WAVE,
              "Enemy count must be 1..50");
      Require(c.row == 0 || c.row == 1, "Red-eye limit must be 0 or 1");
      int gigaWeight = c.col == 0 ? 1000 : c.col;
      Require(gigaWeight >= 1000 && gigaWeight <= 6000,
              "Red-eye weight must be 1000..6000");
      std::vector<std::vector<int>> generated;
      std::mt19937 rng(SDL_GetTicks());
      for (int w = 0; w < b->mNumWaves; ++w) {
        std::vector<int> eligible;
        std::vector<double> weights;
        for (int t : types) {
          if (t == 1 || t == 19 || t == 20)
            continue;
          if (t == 32 && c.row && w % 20 >= 10 && w % 20 <= 18)
            continue;
          eligible.push_back(t);
          double weight =
              GetZombieDefinition(static_cast<ZombieType>(t)).mPickWeight;
          if (t == 0)
            weight = 400;
          if (t == 2)
            weight = 1000;
          if (t == 32)
            weight = b->IsFlagWave(w) ? 6000 : gigaWeight;
          weights.push_back(std::max(1.0, weight));
        }
        Require(!eligible.empty(), "Select a regular zombie for all waves");
        std::discrete_distribution<std::size_t> pick(weights.begin(),
                                                     weights.end());
        std::vector<int> wave;
        for (int i = 0; i < c.extra; ++i)
          wave.push_back(eligible[c.value == 1 ? std::size_t(i + w * c.extra) %
                                                     eligible.size()
                                               : pick(rng)]);
        auto selected = [&](int t) {
          return std::find(types.begin(), types.end(), t) != types.end();
        };
        if (b->IsFlagWave(w)) {
          if (selected(1) || c.value == 2)
            wave[0] = 1;
          if (selected(20))
            for (int i = 9; i < std::min(13, c.extra); ++i)
              wave[i] = 20;
          if (c.value == 2)
            for (int i = 1; i < std::min(9, c.extra); ++i)
              wave[i] = 0;
        }
        generated.push_back(std::move(wave));
      }
      if (std::find(types.begin(), types.end(), 19) != types.end() &&
          !generated.empty())
        generated[rng() % generated.size()][0] = 19;
      for (int w = 0; w < b->mNumWaves; ++w)
        for (int i = 0; i < MAX_ZOMBIES_IN_WAVE; ++i)
          b->mZombiesInWave[w][i] =
              i < c.extra ? static_cast<ZombieType>(generated[w][i])
                          : ZombieType::ZOMBIE_INVALID;
    }
    text = SpawnText(b);
    if (app->mSeedChooserScreen && b->mCutScene) {
      b->RemoveCutsceneZombies();
      b->mCutScene->PlaceStreetZombies();
    }
    break;
  }
  case Action::SpawnRead:
    text = SpawnText(b);
    break;
  case Action::SpawnApply:
    ApplySpawn(b, c.text);
    if (app->mSeedChooserScreen && b->mCutScene) {
      b->RemoveCutsceneZombies();
      b->mCutScene->PlaceStreetZombies();
    }
    break;
  case Action::SpawnSave:
    WriteText(c.text, SpawnText(b));
    break;
  case Action::SpawnLoad:
    ApplySpawn(b, ReadText(c.text));
    text = SpawnText(b);
    if (app->mSeedChooserScreen && b->mCutScene) {
      b->RemoveCutsceneZombies();
      b->mCutScene->PlaceStreetZombies();
    }
    break;
  case Action::Music:
    Require(app->mMusic && c.value >= 1 && c.value < NUM_MUSIC_TUNES,
            "Music must be 1..13");
    app->mMusic->MakeSureMusicIsPlaying(static_cast<MusicTune>(c.value));
    break;
  case Action::SaveFolder: {
    Require(RevealFolder(Sexy::GetAppDataPath("userdata")),
            "Cannot open save directory on this platform");
    break;
  }
  case Action::Pack:
  case Action::Unpack: {
    auto split = c.text.find('\n');
    Require(split != std::string::npos,
            "Enter source path, newline, destination path");
    auto source = std::filesystem::u8path(c.text.substr(0, split));
    auto dest = std::filesystem::u8path(c.text.substr(split + 1));
    if (c.action == Action::Pack)
      Formats::Pack(source, dest);
    else
      Formats::Unpack(source, dest);
    break;
  }
  case Action::Debug:
    Require(c.value >= 0 && c.value <= 4, "Debug mode must be 0..4");
    b->mDebugTextMode = static_cast<DebugTextMode>(c.value);
    break;
  case Action::Speed:
    Require(c.value == 0 || (c.value >= 10 && c.value <= 1000),
            "Speed must be 0 or 10..1000 percent");
    paused = c.value == 0;
    if (c.value)
      speed = c.value / 100.0;
    break;
  case Action::Reset:
    AllOff();
    break;
  case Action::Documentation:
    Require(SDL_OpenURL("https://pvz.lmintlcx.com/toolkit/") == 0,
            "Cannot open documentation on this platform");
    break;
  case Action::About:
    text =
        chinese
            ? "PvZ-Portable 工具箱\n参考 PvZ Toolkit 1.22 "
              "功能，基于游戏源码独立实现。\n内置模块，使用原生游戏 "
              "GUI；不支持修改其他游戏进程。\n关闭效果不会撤销已修改的数值、植"
              "物或用户进度。\n源码：https://github.com/lmintlcx/pvztoolkit"
            : "PvZ-Portable Toolkit\nBehavior reference: PvZ Toolkit 1.22; "
              "independently implemented against game source.\nBuilt-in module "
              "using the game's own GUI, for this game instance "
              "only.\nDisabling effects does not undo changed values, objects "
              "or profile progress.\nReference: "
              "https://github.com/lmintlcx/pvztoolkit";
    break;
  case Action::Screenshot:
    screenshotRequested = true;
    app->mWidgetManager->MarkAllDirty();
    messageEn = "Screenshot requested";
    messageZh = "已请求截图";
    break;
  }
}
} // namespace Trainer
#endif
