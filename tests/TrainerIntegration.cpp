// SPDX-License-Identifier: LGPL-3.0-or-later
// Real-engine integration checks. Run only with an isolated -savedir (runner below).
#include "LawnApp.h"
#include "Resources.h"
#include "PvzpLib/PvzpStringFile.h"
#include "Lawn/Board.h"
#include "Lawn/Plant.h"
#include "Lawn/Zombie.h"
#include "Lawn/Coin.h"
#include "Lawn/SeedPacket.h"
#include "Lawn/Cutscene.h"
#include "Lawn/System/PlayerInfo.h"
#include "Lawn/System/ProfileMgr.h"
#include "Lawn/Widget/TitleScreen.h"
#include "Trainer/Trainer.h"
#include "Trainer/TrainerFormats.h"
#include "Trainer/TrainerRuntime.h"
#include "Trainer/TrainerPanel.h"
#include "Trainer/TrainerClassicLayout.h"
#include "Trainer/TrainerFont.h"
#include "widget/WidgetManager.h"
#include "widget/ButtonWidget.h"
#include "imagelib/ImageLib.h"
#include <algorithm>
#include <SDL.h>
#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
int failures = 0;
int checks = 0;
void Check(bool pass, const char* name) {
 ++checks;
 std::cout << (pass ? "PASS " : "FAIL ") << name << std::endl;
 if (!pass) ++failures;
}
std::string Read(const std::filesystem::path& p) {
 std::ifstream f(p, std::ios::binary);
 return {std::istreambuf_iterator<char>(f), {}};
}
class IntegrationApp final : public LawnApp {
 int stage = 0;
 unsigned start = SDL_GetTicks();
 void Apply(Trainer::Action action, int value = 0, int row = 0, int col = 0, int extra = 0, std::string text = {}) {
  auto& s = mTrainer->GetService();
  s.Submit({action, value, row, col, extra, std::move(text)});
  s.Tick();
 }
 void RunChecks() {
  using namespace Trainer;
  auto& s = mTrainer->GetService();
  auto* b = mBoard;
  s.AllOff();
  s.paused = true;
  Check(s.state.board && s.state.rows == 6, "pool board snapshot");
  Apply(Action::Sun, 4321);
  Check(b->mSunMoney == 4321, "sun command writes actual Board");
  Apply(Action::Money, 12340);
  Check(mPlayerInfo->mCoins == 1234, "currency command uses original ten-coin units");
  Apply(Action::Sun, -1);
  Check(b->mSunMoney == 4321, "invalid sun rejected without changing board");
  s.Set(Flag::PlantInvincible, true); s.Set(Flag::PlantWeak, true);
  Check(!s.Get(Flag::PlantInvincible) && s.Get(Flag::PlantWeak), "plant modes mutually exclusive");
  s.Set(Flag::ZombieWeak, true); s.Set(Flag::ZombieInvincible, true);
  Check(s.Get(Flag::ZombieInvincible) && !s.Get(Flag::ZombieWeak), "zombie modes mutually exclusive");
  s.AllOff();
  b->mSunMoney = 0;
  Check(!b->CanTakeSunMoney(100), "ordinary sun-cost check retained");
  s.Set(Flag::FreePlanting, true);
  Check(b->CanTakeSunMoney(100) && b->TakeSunMoney(100) && b->mSunMoney == 0, "free planting bypasses charge");
  s.Set(Flag::FreePlanting, false);
  Check(!b->CanTakeSunMoney(100), "sun-cost restored after disabling");
  s.Set(Flag::SunLimit, true); b->mSunMoney = 2000000000; b->AddSunMoney(1000000000);
  Check(b->mSunMoney == 2147483647, "unlimited sunlight addition does not overflow");
  s.Set(Flag::SunLimit, false); b->mSunMoney = 9990; b->AddSunMoney(25);
  Check(b->mSunMoney == 9990, "vanilla sunlight cap restored");
  auto savedFertilizer = mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_FERTILIZER];
  auto savedSpray = mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_BUG_SPRAY];
  s.Set(Flag::Fertilizer, true); s.Set(Flag::BugSpray, true); s.Tick();
  auto savedMode = mGameMode;
  mGameMode = GameMode::GAMEMODE_CHALLENGE_ZEN_GARDEN;
  Check(b->CanUseGameObject(GameObjectType::OBJECT_TYPE_FERTILIZER) &&
        b->CanUseGameObject(GameObjectType::OBJECT_TYPE_BUG_SPRAY),
        "unlimited garden tools become available without buying stock");
  mGameMode = savedMode;
  Check(mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_FERTILIZER] == savedFertilizer &&
        mPlayerInfo->mPurchases[StoreItem::STORE_ITEM_BUG_SPRAY] == savedSpray,
        "unlimited garden toggles do not mutate persistent inventory");
  s.AllOff(); s.paused = true;
  auto& packet = b->mSeedBank->mSeedPackets[0];
  packet.SetPacketType(SeedType::SEED_PEASHOOTER);
  packet.mActive = false; packet.mRefreshing = true; packet.mRefreshTime = 750; packet.mRefreshCounter = 0;
  packet.Update();
  Check(!packet.mActive && packet.mRefreshCounter == 1, "ordinary card refresh still advances naturally");
  s.Set(Flag::NoCooldown, true); packet.Update();
  Check(packet.mActive && !packet.mRefreshing, "card cooldown hook activates real packet");
  s.Set(Flag::NoCooldown, false);
  Check(b->CanPlantAt(8, 0, SeedType::SEED_COBCANNON) != PlantingReason::PLANTING_OK, "ordinary cob boundary restriction");
  s.Set(Flag::Anywhere, true);
  Check(b->CanPlantAt(8, 0, SeedType::SEED_COBCANNON) != PlantingReason::PLANTING_OK, "placement bypass retains cob boundary");
  Check(b->CanPlantAt(0, 2, SeedType::SEED_PEASHOOTER) == PlantingReason::PLANTING_OK, "placement bypass allows ordinary plant on water");
  s.Set(Flag::Anywhere, false);
  int countBeforeInvalidCob = b->mPlants.mSize;
  Apply(Action::Plant, int(SeedType::SEED_COBCANNON), 0, 8);
  Check(b->mPlants.mSize == countBeforeInvalidCob,
        "service rejects two-cell cob at final column before creating objects");
  Apply(Action::ClearPlants);
  Apply(Action::Plant, int(SeedType::SEED_SUNFLOWER), 0, 0);
  Plant* plant = b->GetTopPlantAt(0, 0, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION);
  Check(plant != nullptr, "plant command creates actual plant");
  Apply(Action::Zombie, int(ZombieType::ZOMBIE_NORMAL), 0, 7);
  Zombie* zombie = nullptr;
  for (auto* z : b->mZombies) if (!z->mDead && z->IsOnBoard()) { zombie = z; break; }
  Check(zombie != nullptr, "zombie command creates actual zombie");
  if (plant && zombie) {
   int before = zombie->mBodyHealth;
   s.Set(Flag::ZombieInvincible, true); zombie->TakeDamage(20, 0);
   Check(zombie->mBodyHealth == before, "invincible zombie retains health");
   s.Set(Flag::ZombieInvincible, false); zombie->TakeDamage(20, 0);
   Check(zombie->mBodyHealth < before, "zombie damage restored");
   before = plant->mPlantHealth;
   s.Set(Flag::PlantInvincible, true); zombie->EatPlant(plant);
   Check(plant->mPlantHealth == before, "invincible plant resists bite");
   s.Set(Flag::PlantInvincible, false); zombie->EatPlant(plant);
   Check(plant->mPlantHealth < before, "plant bite damage restored");
  }
  auto* coin = b->AddCoin(300, 150, CoinType::COIN_SUN, CoinMotion::COIN_MOTION_COIN);
  s.Set(Flag::AutoCollect, true); coin->Update();
  Check(coin->mIsBeingCollected, "autocollect invokes real coin collection");
  s.AllOff();
  Apply(Action::Scene, 0);
  Check(!b->StageHas6Rows() && b->mPlantRow[5] == PlantRowType::PLANTROW_DIRT, "scene transition reduces row topology");
  Apply(Action::Scene, 2);
  Check(b->StageHasPool() && b->mPlantRow[2] == PlantRowType::PLANTROW_POOL, "pool scene restores water topology");
  Apply(Action::Plant, int(SeedType::SEED_SUNFLOWER), 0, 0, 1);
  Apply(Action::ExportLineup);
  std::string code = s.text;
  auto lineup = Formats::DecodeLineup(code);
  Check((lineup.cells[0] & 512) != 0, "lineup export preserves imitater identity");
  // Repeated imports while paused must release deferred objects instead of exhausting DataArray.
  for (int i = 0; i < 10; ++i) Apply(Action::ImportLineup, 0, 0, 0, 0, code);
  Apply(Action::ExportLineup);
  Check(s.text == code && b->mPlants.mSize < 10, "paused repeated lineup application stays bounded and lossless");
  Apply(Action::SpawnGenerate, 1, 0, 1000, 5, "0 2 4");
  bool validWaves = true;
  for (int wave = 0; wave < b->mNumWaves; ++wave) {
   for (int i = 0; i < 5; ++i) {
    int type = int(b->mZombiesInWave[wave][i]);
    validWaves = validWaves && (type == 0 || type == 2 || type == 4);
   }
   validWaves = validWaves && b->mZombiesInWave[wave][5] == ZombieType::ZOMBIE_INVALID;
  }
  Check(validWaves, "uniform spawn command generates bounded requested waves");
  Apply(Action::SpawnRead);
  auto spawnText = s.text;
  Apply(Action::SpawnApply, 0, 0, 0, 0, spawnText);
  Apply(Action::SpawnRead);
  Check(s.text == spawnText, "spawn editor roundtrip retains full wave list");
  auto firstType = b->mZombiesInWave[0][0];
  Apply(Action::SpawnGenerate, 2, 0, 1000, 5, "25");
  Check(b->mZombiesInWave[0][0] == firstType, "boss spawn request rejected before changing waves");
  auto oldGeneration = s.state.generation;
  Apply(Action::Sun, 321);
  s.Submit({Action::Sun, 888});
  mTrainer->InvalidateBoard();
  s.Tick();
  Check(b->mSunMoney == 321 && s.state.generation != oldGeneration, "stale queued commands discarded on board invalidation");
  auto path = Sexy::GetAppDataPath("userdata/trainer-integration.dat");
  Check(WriteBytesToFile(path, "old", 3), "prepare isolated save fixture");
  s.Set(Flag::Readonly, true);
  WriteBytesToFile(path, "new", 3); EraseFile(path);
  Check(Read(path) == "old", "readonly blocks both save writes and deletes");
  auto cache = Sexy::GetAppDataPath("trainer-integration-settings.dat");
  Check(WriteBytesToFile(cache, "ok", 2) && Read(cache) == "ok", "readonly permits independent settings");
  s.Set(Flag::Readonly, false); EraseFile(path);
  Check(!std::filesystem::exists(path), "save delete restored");
  s.Set(Flag::FreePlanting, true);
  mRecordingDemoBuffer = true;
  mTrainer->Tick();
  Check(!s.panelOpen, "recording hides toolkit UI");
  SDL_Event forbidden{}; forbidden.type = SDL_KEYDOWN; forbidden.key.keysym.sym = SDLK_F10;
  mTrainer->Event(forbidden);
  Check(!s.panelOpen, "F10 cannot reopen toolkit during demo recording");
  Check(!Enabled(Flag::FreePlanting), "recording disables active gameplay flags");
  s.Set(Flag::AutoCollect, true);
  Check(!s.Get(Flag::AutoCollect), "recording rejects new toggles");
  s.Submit({Action::Sun, 999}); s.Tick();
  Check(b->mSunMoney == 321, "recording rejects modifying commands");
  mRecordingDemoBuffer = false; s.Tick(); s.AllOff();
  if (!s.panelOpen) mTrainer->Toggle();
  s.paused = true;
  auto mobilePause = s.mobilePause;
  s.mobilePause = false;
  Check(mTrainer->UpdateCount(1) == 0, "explicit trainer pause stops simulation ticks");
  s.paused = false; s.speed = 2;
  Check(mTrainer->UpdateCount(1) == 2, "2x speed schedules two complete ticks");
  s.speed = 0.5;
  int slowTicks = mTrainer->UpdateCount(1) + mTrainer->UpdateCount(1);
  Check(slowTicks == 1, "fractional speed accumulates whole simulation ticks");
  s.paused = true; s.speed = 1; s.mobilePause = mobilePause;
  Apply(Action::Screenshot);
  Check(s.screenshotRequested, "game screenshot is queued for next present");
  mTrainer->Tick();
  if (mTrainerMobile) Check(!s.panelOpen, "mobile screenshot hides tools before drawing lawn");
  mNextDrawTick = SDL_GetTicks();
  DrawDirtyStuff();
  Check(!s.screenshotRequested && std::filesystem::is_regular_file(Sexy::PathFromU8(s.text)),
        "dirty redraw consumes screenshot and writes actual game PNG");
  std::unique_ptr<ImageLib::Image> gameImage(ImageLib::GetImage(s.text, false));
  bool nonuniform = false;
  if (gameImage && gameImage->mBits && gameImage->mWidth > 0 && gameImage->mHeight > 0) {
   auto* pixels = gameImage->GetBits();
   nonuniform = std::any_of(pixels + 1, pixels + static_cast<size_t>(gameImage->mWidth) * gameImage->mHeight,
                           [&](uint32_t pixel) { return pixel != pixels[0]; });
  }
  Check(nonuniform, "saved game screenshot decodes to nonuniform rendered content");
  if (!s.panelOpen) mTrainer->Toggle();
  auto& panel = mTrainer->GetPanel();
  auto out = std::filesystem::path(std::getenv("PVZ_TRAINER_TEST_OUT"));
  std::cout << "FONT " << Trainer::FontSource() << std::endl;
  Check(Trainer::HasChineseFont(), "installed font supports complete Chinese toolkit text");
  namespace C = Trainer::Classic;
  panel.SetValue(C::Language, 0); panel.ButtonDepress(C::Language); panel.Update();
  SDL_Delay(35); mTrainer->Pump(); panel.Update();
  if (auto windowId = mTrainer->WindowId()) {
   int width = 0, height = 0; SDL_GetWindowSize(SDL_GetWindowFromID(windowId), &width, &height);
   Check(width == 450 && height == 275, "Chinese native tool window matches compact reference size");
  }
  Check(panel.PreferredWidth() == 450 && panel.PreferredHeight() == 275,
        "Chinese panel uses exact compact Toolkit reference canvas");
  Check(Read(Sexy::GetAppDataPath("trainer/settings-v1.txt")).find("language 1") != std::string::npos,
        "Chinese preference persists outside original game save");
  Apply(Action::ClearPlants);
  panel.ButtonDepress(11); panel.Refresh();
  panel.ButtonDepress(C::Plant);
  Check(mTrainer->SavePanelImage((out / "toolkit-types-zh.png").string()),
        "render original compact plant dropdown in Chinese");
  panel.KeyDown(Sexy::KEYCODE_ESCAPE);
  panel.SetValue(C::Row, 1); panel.SetValue(C::Col, 1);
  panel.SetValue(C::Plant, int(SeedType::SEED_SUNFLOWER));
  panel.SetValue(C::Imitater, 0); panel.ButtonDepress(C::PutPlant); s.Tick();
  auto* chosenPlant = b->GetTopPlantAt(0, 0, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION);
  Check(chosenPlant && chosenPlant->mSeedType == SeedType::SEED_SUNFLOWER &&
        chosenPlant->mImitaterType == SeedType::SEED_NONE,
        "direct compact plant controls create selected sunflower");
  panel.ButtonDepress(13); panel.Refresh();
  for (int i = 0; i < 20; ++i) panel.SetValue(C::SpawnFirst + i, 0);
  panel.ButtonDepress(C::SpawnFirst); panel.ButtonDepress(C::SpawnFirst + 2);
  if (panel.embedded) {
    panel.ButtonPress(C::Generate);
    for (int i = 0; i < 40; ++i) panel.ButtonDownTick(C::Generate);
    panel.ButtonDepress(C::Generate);
    panel.ButtonDepress(5002);
    Check(panel.Value(C::SpawnMode) == 2, "touch long-press opens selectable spawning algorithm menu");
  }
  panel.SetValue(C::SpawnMode, 1); panel.ButtonDepress(C::Generate); s.Tick();
  bool selectedSpawnTypes = true;
  for (int wave = 0; wave < b->mNumWaves; ++wave)
   for (int i = 0; i < MAX_ZOMBIES_IN_WAVE; ++i) {
    auto type = b->mZombiesInWave[wave][i];
    selectedSpawnTypes = selectedSpawnTypes &&
        (type == ZombieType::ZOMBIE_NORMAL || type == ZombieType::ZOMBIE_TRAFFIC_CONE ||
         type == ZombieType::ZOMBIE_PAIL);
   }
  Check(panel.Value(C::SpawnFirst) == 1 && panel.Value(C::SpawnFirst + 2) == 1 && selectedSpawnTypes,
        "classic zombie checkbox grid generates selected enemy types");
  panel.ButtonDepress(C::Details); s.Tick();
  Check(panel.detailsOpen && !s.text.empty(), "classic details button opens actual spawn statistics");
  panel.KeyDown(Sexy::KEYCODE_ESCAPE);
  for (int i = 0; i < 5; ++i) {
   panel.ButtonDepress(10 + i); panel.Refresh();
   Check(mTrainer->SavePanelImage((out / ("toolkit-page-zh-" + std::to_string(i) + ".png")).string()),
         "render compact Chinese Toolkit category");
  }
  panel.ButtonDepress(10); panel.Refresh();
  Check(mTrainer->SavePanelImage((out / "toolkit-zh.png").string()), "render Chinese toolkit panel");
  panel.SetValue(C::Language, 1); panel.ButtonDepress(C::Language); panel.Update();
  SDL_Delay(35); mTrainer->Pump(); panel.Update();
  if (auto windowId = mTrainer->WindowId()) {
   int width = 0, height = 0; SDL_GetWindowSize(SDL_GetWindowFromID(windowId), &width, &height);
   Check(width == 536 && height == 280, "English native tool window resizes to original wider geometry");
  }
  Check(!s.chinese && panel.PreferredWidth() == 536 && panel.PreferredHeight() == 280,
        "language selector switches to original wider English layout");
  Check(Read(Sexy::GetAppDataPath("trainer/settings-v1.txt")).find("language 0") != std::string::npos,
        "language selector persists English preference immediately");
  Check(mTrainer->SavePanelImage((out / "toolkit-en.png").string()), "render English toolkit panel");
  for (int i = 0; i < 5; ++i) {
   panel.ButtonDepress(10 + i); panel.Refresh();
   Check(mTrainer->SavePanelImage((out / ("toolkit-page-" + std::to_string(i) + ".png")).string()),
         "render compact English Toolkit category");
  }
  panel.ButtonDepress(10); panel.Refresh();
  auto id = mTrainer->WindowId();
  if (id) {
   panel.SetValue(C::SunInput, 2468);
   Sexy::ButtonWidget* sunButton = nullptr;
   for (auto* child : panel.mWidgets)
    if (auto* button = dynamic_cast<Sexy::ButtonWidget*>(child); button && button->mId == C::SetSun)
     sunButton = button;
   if (!sunButton) throw std::runtime_error("Compact sunlight button not found");
   auto point = sunButton->GetAbsPos();
   SDL_Event click{}; click.type = SDL_MOUSEBUTTONDOWN; click.button.windowID = id;
   click.button.button = SDL_BUTTON_LEFT;
   click.button.x = point.mX + sunButton->mWidth / 2;
   click.button.y = point.mY + sunButton->mHeight / 2;
   SDL_Event focus{}; focus.type = SDL_WINDOWEVENT; focus.window.windowID = id;
   focus.window.event = SDL_WINDOWEVENT_FOCUS_GAINED; mTrainer->Event(focus);
   SDL_Event motion{}; motion.type = SDL_MOUSEMOTION; motion.motion.windowID = id;
   motion.motion.x = click.button.x; motion.motion.y = click.button.y;
   mTrainer->Event(motion);
   Check(mTrainer->Event(click), "toolkit window consumes its own mouse press");
   click.type = SDL_MOUSEBUTTONUP;
   mTrainer->Event(click); s.Tick();
   Check(b->mSunMoney == 2468, "real mouse route activates direct sunlight button");
   SDL_Event e{}; e.type = SDL_WINDOWEVENT; e.window.windowID = id; e.window.event = SDL_WINDOWEVENT_CLOSE;
   Check(mTrainer->Event(e) && !s.panelOpen && !mShutdown && !mCloseRequest, "closing second window keeps game alive");
   mTrainer->Toggle();
   Check(s.panelOpen, "second window reopens");
   e.window.windowID = SDL_GetWindowID(static_cast<SDL_Window*>(mWindow));
   Check(!mTrainer->Event(e), "game window events are not consumed by tool window");
  } else {
   Check(mTrainerMobile && s.panelOpen, "forced mobile overlay available");
   mTrainer->Toggle();
   Check(!s.mobilePause, "closing mobile panel releases only its own pause");
  }
  if (!s.panelOpen) mTrainer->Toggle();
  panel.SetValue(C::Plant, int(SeedType::SEED_PEASHOOTER)); panel.SetValue(C::Imitater, 0);
  panel.ButtonDepress(12); panel.Refresh(); panel.ButtonDepress(C::Quick); mTrainer->Pump();
  SDL_Event place{}; place.type = SDL_MOUSEBUTTONDOWN; place.button.button = SDL_BUTTON_LEFT;
  place.button.windowID = SDL_GetWindowID(static_cast<SDL_Window*>(mWindow));
  auto source = mWidgetManager->mMouseSourceRect;
  auto dest = mWidgetManager->mMouseDestRect;
  place.button.x = source.mX + (b->GridToPixelX(4, 0) + 40 - dest.mX) * source.mWidth / dest.mWidth;
  place.button.y = source.mY + (b->GridToPixelY(4, 0) + 40 - dest.mY) * source.mHeight / dest.mHeight;
  Check(mTrainer->Event(place), "placement mode consumes game-window press");
  s.Tick();
  Check(b->GetTopPlantAt(4, 0, PlantPriority::TOPPLANT_ONLY_NORMAL_POSITION) != nullptr,
        "placement click creates plant in selected grid cell");
  place.type = SDL_MOUSEBUTTONUP;
  Check(mTrainer->Event(place), "placement mode also consumes release to prevent normal planting");
  place.type = SDL_MOUSEBUTTONDOWN; place.button.button = SDL_BUTTON_RIGHT;
  Check(mTrainer->Event(place), "right click finishes continuous placement");
  place.type = SDL_MOUSEBUTTONUP;
  mTrainer->Event(place);
  place.type = SDL_MOUSEBUTTONDOWN; place.button.button = SDL_BUTTON_LEFT;
  Check(!mTrainer->Event(place), "normal game input resumes after leaving placement mode");
  if (panel.Value(C::Quick)) panel.ButtonDepress(C::Quick);
  oldGeneration = s.state.generation;
  s.Submit({Action::Sun, 888});
  PreNewGame(GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_3, false);
  mTrainer->GetService().Tick();
  Check(mBoard->mSunMoney != 888 && s.state.generation != oldGeneration,
        "actual level replacement invalidates commands even when allocation reuses address");
  panel.ButtonDepress(10); panel.Refresh();
  panel.SetValue(C::Mode, int(GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_3));
  Check(panel.Value(C::Mode) == int(GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_3),
        "original mixed-mode selector retains chosen game mode");
  panel.ButtonDepress(C::Mode);
  Check(mTrainer->SavePanelImage((out / "toolkit-modes.png").string()), "render original game-mode dropdown");
  panel.KeyDown(Sexy::KEYCODE_ESCAPE);
  panel.ButtonDepress(C::Mix); s.Tick();
  Check(mGameMode == GameMode::GAMEMODE_SURVIVAL_NORMAL_STAGE_3,
        "direct mixed-mode button starts selected pool survival level");
  s.AllOff(); s.paused = false;
 }
public:
 void UpdateFrames() override {
  LawnApp::UpdateFrames();
  try {
   if (SDL_GetTicks() - start > 45000) throw std::runtime_error("test timed out waiting for resources");
   if (!mTrainer || !mLoadingThreadCompleted) return;
   if (stage == 0) {
    mPlayerInfo = mProfileMgr->AddProfile("Toolkit integration");
#ifdef PVZ_DEFAULT_UNLOCKED
    Check(mPlayerInfo->mFinishedAdventure >= 2 && mPlayerInfo->mPurchases[STORE_ITEM_PACKET_UPGRADE] >= 4,
          "new profile defaults to unlocked content");
#endif
    mPlayerInfo->mCoins = 123;
    mPlayerInfo->mChallengeRecords[0] = 42;
    mPlayerInfo->UnlockContent();
    Check(mPlayerInfo->mFinishedAdventure >= 2 && mPlayerInfo->mHasUnlockedPuzzleMode &&
          mPlayerInfo->mPurchases[STORE_ITEM_PLANT_IMITATER] &&
          mPlayerInfo->mPurchases[STORE_ITEM_PACKET_UPGRADE] == 4 &&
          mPlayerInfo->mPurchases[STORE_ITEM_AQUARIUM_GARDEN],
          "unlock covers modes, plants, ten slots and garden upgrades");
    mPlayerInfo->SaveDetails();
    mPlayerInfo->Reset();
    mPlayerInfo->LoadDetails();
    Check(mPlayerInfo->mFinishedAdventure >= 2 && mPlayerInfo->mChallengeRecords[0] == 42 &&
          mPlayerInfo->mCoins == 123 && mPlayerInfo->mPurchases[STORE_ITEM_PACKET_UPGRADE] == 4,
          "unlocked profile persists without reducing records or replacing currency");
    mPlayerInfo->mFinishedAdventure = 2;
    mPlayerInfo->mLevel = 50;
    if (mTitleScreen) LoadingCompleted();
    KillGameSelector();
    PreNewGame(GameMode::GAMEMODE_SURVIVAL_ENDLESS_STAGE_3, false);
    mBoard->mCutScene->CancelIntro();
    StartPlaying();
    mBoard->Move(0, 0);
    mTrainer->GetService().paused = true;
    mTrainer->GetService().Tick();
    ++stage;
   } else if (stage == 1) {
    ++stage;
    RunChecks();
    std::cout << "RESULT checks=" << checks << " failures=" << failures << std::endl;
    CloseRequestAsync();
   }
  } catch (const std::exception& e) {
   std::cerr << "FAIL exception: " << e.what() << std::endl;
   ++failures; stage = 2; CloseRequestAsync();
  }
 }
};
}
int main(int argc, char** argv) {
 if (!std::getenv("PVZ_TRAINER_TEST_OUT")) {
  std::cerr << "Use tests/run-trainer-integration.py to isolate saves." << std::endl;
  return 2;
 }
 PvzpStringListSetColors(gLawnStringFormats, gLawnStringFormatCount);
 gExtractResourcesByName = Sexy::ExtractResourcesByName;
 auto* app = new IntegrationApp(); gLawnApp = app;
 app->SetArgs(argc, argv); app->Init(); app->Start(); app->Shutdown(); delete app;
 return failures ? 1 : 0;
}
