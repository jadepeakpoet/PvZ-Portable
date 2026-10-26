// SPDX-License-Identifier: LGPL-3.0-or-later
// Independently authored layout metadata matching the PvZ Toolkit 1.22 UI.
// Geometry and control naming reference: lmintlcx/pvztoolkit, src/window.cpp
// https://github.com/lmintlcx/pvztoolkit (GPL-3.0). No FLTK/backend code is
// used.
#include "TrainerClassicLayout.h"

namespace Trainer::Classic {
std::vector<Control> Layout(bool chinese) {
  const int m = chinese ? 10 : 8;
  const int iw = chinese ? 95 : 120;
  const int ih = chinese ? 25 : 24;
  const int x = m * 2;
  const int y = m + (chinese ? 30 : 27) + 5 + m;
  auto c = [=](int col) { return x + (iw + m) * (col - 1); };
  auto r = [=](int row) { return y + (ih + m) * (row - 1); };
  std::vector<Control> result;
  auto add = [&](int id, int page, Kind kind, int px, int py, int w, int h,
                 const char *label = "") {
    result.push_back({id, page, kind, px, py, w, h, label});
  };
  auto flag = [&](Flag f, int page, int px, int py, int w, const char *label) {
    add(FlagFirst + static_cast<int>(f), page, Kind::Check, px, py, w, ih,
        label);
  };
  if (chinese) {
    // Resources: four columns, six rows, exactly the compact Chinese window.
    flag(Flag::SunLimit, 0, c(1) + 8, r(1), iw - 76, "");
    add(SunInput, 0, Kind::Input, c(1) + 45, r(1), iw - 15, ih);
    add(SetSun, 0, Kind::Button, c(2) + 30, r(1), iw - 45, ih, "阳光");
    add(-1, 0, Kind::Label, c(1) + 8, r(2), iw - 76, ih, "钱包");
    add(MoneyInput, 0, Kind::Input, c(1) + 45, r(2), iw - 15, ih);
    add(SetMoney, 0, Kind::Button, c(2) + 30, r(2), iw - 45, ih, "金币");
    flag(Flag::Fertilizer, 0, c(3), r(1), iw - 15, "花肥无限");
    flag(Flag::BugSpray, 0, c(4), r(1), iw, "杀虫剂无限");
    flag(Flag::TreeFood, 0, c(3), r(2), iw - 15, "树肥无限");
    flag(Flag::Chocolate, 0, c(4), r(2), iw, "巧克力无限");
    flag(Flag::AutoCollect, 0, c(3), r(3), iw - 15, "自动收集");
    flag(Flag::NoLoot, 0, c(4), r(3), iw, "不掉战利品");
    add(-2, 0, Kind::Label, c(1) + 8, r(3), iw - 76, ih, "树高");
    add(TreeInput, 0, Kind::Input, c(1) + 45, r(3), iw - 15, ih);
    add(SetTree, 0, Kind::Button, c(2) + 30, r(3), iw - 45, ih, "英尺");
    flag(Flag::FreePlanting, 0, c(1), r(4), iw - 15, "免费用卡");
    flag(Flag::Anywhere, 0, c(2), r(4), iw - 15, "随意放置");
    flag(Flag::FastBelt, 0, c(3), r(4), iw - 15, "无缝传送");
    flag(Flag::LockShovel, 0, c(4), r(4), iw - 15, "连续铲子");
    add(Mode, 0, Kind::Choice, c(1), r(5), iw * 2 + m + 20, ih);
    add(Adventure, 0, Kind::Choice, c(3) + 20, r(5), iw - 5, ih);
    add(Mix, 0, Kind::Button, c(4) + 15, r(5), iw - 15, ih, "混乱关卡");
    add(Rounds, 0, Kind::Input, c(3) + 20, r(6), iw - 5, ih);
    add(SetRounds, 0, Kind::Button, c(4) + 15, r(6), iw - 15, ih, "无尽轮数");
    add(Unlock, 0, Kind::Button, c(1), r(6), iw + 8, ih, "通关存档");
    add(BrightestCob, 0, Kind::Check, c(2) + 14, r(6), 18, ih);
    add(Win, 0, Kind::Button, c(2) + 35, r(6), iw - 15, ih, "直接过关");

    // Battlefield.
    add(Row, 1, Kind::Choice, c(1), r(1), iw - 10, ih);
    add(Col, 1, Kind::Choice, c(1), r(2), iw - 10, ih);
    add(Plant, 1, Kind::Choice, c(2) - 10, r(1), iw + 50, ih);
    add(Zombie, 1, Kind::Choice, c(2) - 10, r(2), iw + 50, ih);
    add(PutPlant, 1, Kind::Button, c(3) + 40, r(1), iw - 20, ih, "种植物");
    add(Imitater, 1, Kind::Check, c(4) + 20, r(1), iw - 20, ih, "模仿者");
    add(PutZombie, 1, Kind::Button, c(3) + 40, r(2), iw - 20, ih, "出僵尸");
    add(Grave, 1, Kind::Button, c(4) + 20, r(2), iw - 20, ih, "冒墓碑");
    add(Ladder, 1, Kind::Button, c(3) + 40, r(3), iw - 20, ih, "搭梯子");
    add(Rake, 1, Kind::Button, c(4) + 20, r(3), iw - 20, ih, "放钉耙");
    add(Mowers, 1, Kind::Choice, c(1), r(3), iw - 10, ih, "小推车");
    add(ClearType, 1, Kind::Choice, c(2) - 10, r(3), iw - 10, ih);
    add(Clear, 1, Kind::Button, c(2) + iw - 20 + m, r(3), iw - 45, ih, "清除");
    flag(Flag::PlantInvincible, 1, c(1), r(4), iw - 15, "植物无敌");
    flag(Flag::PlantWeak, 1, c(2), r(4), iw - 15, "植物脆弱");
    flag(Flag::ZombieInvincible, 1, c(3), r(4), iw - 15, "僵尸无敌");
    flag(Flag::ZombieWeak, 1, c(4), r(4), iw - 15, "僵尸虚弱");
    flag(Flag::Reload, 1, c(1), r(5), iw - 15, "立刻装填");
    flag(Flag::Butter, 1, c(2), r(5), iw - 15, "只投黄油");
    flag(Flag::StopSpawning, 1, c(3), r(5), iw - 15, "暂停刷怪");
    flag(Flag::StopZombies, 1, c(4), r(5), iw - 15, "停滞不前");
    flag(Flag::Awake, 1, c(1), r(6), iw - 15, "蘑菇睡醒");
    flag(Flag::NoCrater, 1, c(2), r(6), iw - 15, "核弹无坑");
    flag(Flag::NoIce, 1, c(3), r(6), iw - 15, "冰车无痕");
    flag(Flag::NoExplode, 1, c(4), r(6), iw - 15, "丑椒不爆");

    // Lineup list chooser and load button occupy the same slot; host toggles
    // them.
    add(Slot, 2, Kind::Choice, c(1), r(1), iw - 10, ih);
    add(Seed, 2, Kind::Choice, c(2) - 10, r(1), iw + 50, ih);
    add(SetSlot, 2, Kind::Button, c(3) + 40, r(1), iw - 20, ih, "换卡片");
    add(SeedImitater, 2, Kind::Check, c(4) + 20, r(1), iw - 20, ih, "模仿者");
    add(Quick, 2, Kind::Check, c(1), r(2), iw + 12, ih, "快捷布阵模式");
    add(AutoLadder, 2, Kind::Button, c(2) + 12, r(2), iw - 12, ih, "智能搭梯");
    add(Lily, 2, Kind::Choice, c(3), r(2), iw, ih, "水路睡莲");
    add(Pot, 2, Kind::Choice, c(4), r(2), iw, ih, "屋顶花盆");
    add(ResetScene, 2, Kind::Button, c(1), r(3), iw + 12, ih, "重置场地");
    add(Scene, 2, Kind::Choice, c(2) + 12, r(3), iw - 12, ih);
    add(LoadLineup, 2, Kind::Button, c(3), r(3), iw * 2 + 10, ih,
        "加载阵型列表文件 (***.yml)");
    add(LineupFile, 2, Kind::Choice, c(3), r(3), iw * 2 + 10, ih);
    add(LineupText, 2, Kind::Text, c(1), r(4), iw * 4 + 30, ih * 2 + 12);
    add(GetLineup, 2, Kind::Button, c(1), r(6), iw - 15, ih, "获取代码");
    add(CopyLineup, 2, Kind::Button, c(2) - 15, r(6), iw - 15, ih, "复制导出");
    add(PasteLineup, 2, Kind::Button, c(3) + 30, r(6), iw - 15, ih, "粘贴导入");
    add(ApplyLineup, 2, Kind::Button, c(4) + 15, r(6), iw - 15, ih, "布置阵型");
    add(Screenshot, 2, Kind::Button, c(3) - 30, r(6), 60 - m, ih, "截图");

    // The Chinese reference uses 4x5 head/check pairs.
    constexpr const char *labels[20] = {"路障", "撑杆", "铁桶", "读报", "铁门",
                                        "橄榄", "舞王", "潜水", "冰车", "海豚",
                                        "小丑", "气球", "矿工", "跳跳", "雪人",
                                        "蹦极", "扶梯", "投篮", "白眼", "红眼"};
    for (int row = 0; row < 4; ++row)
      for (int col = 0; col < 5; ++col) {
        const int i = row * 5 + col;
        add(SpawnFirst + i, 3, Kind::Check, x + 8 + (iw - 22 + m) * col,
            y + 5 + (ih + 7 + m) * row, iw - 42, ih, labels[i]);
      }
    add(Details, 3, Kind::Button, c(1), r(6), iw, ih, "查看详情");
    add(GigaWeight, 3, Kind::Choice, c(2) + 5, r(6), iw - 20, ih);
    add(GigaLimit, 3, Kind::Check, c(3) - 15, r(6), iw - 40, ih, "变速");
    add(Generate, 3, Kind::Button, c(4) - 55, r(6), iw + 50, ih,
        "极限出怪 (均匀填充)");
    add(SpawnMode, 3, Kind::Choice, c(4) - 55, r(6), iw + 50, ih,
        "选择刷怪模式：");

    // Miscellaneous.
    add(Music, 4, Kind::Choice, c(1), r(1), iw * 2 + m - 15, ih);
    add(PlayMusic, 4, Kind::Button, c(3) - 15, r(1), iw, ih, "背景音乐");
    add(SaveFolder, 4, Kind::Button, c(4) - 15, r(1), iw + 15, ih,
        "存档文件夹");
    flag(Flag::NoFog, 4, c(1), r(2), iw - 15, "清除浓雾");
    flag(Flag::SeeVase, 4, c(2) - 15, r(2), iw - 15, "罐子透视");
    flag(Flag::Background, 4, c(3) - 30, r(2), iw + 15, "允许后台运行");
    flag(Flag::Readonly, 4, c(4) - 15, r(2), iw + 15, "禁止存档删档");
    add(ChooseFile, 4, Kind::Button, c(1), r(3), iw - 20, ih, "选择文件");
    add(File, 4, Kind::Input, c(2) - 20, r(3), iw * 2 + m + 65, ih);
    add(Unpack, 4, Kind::Button, c(4) + 45, r(3), iw - 45, ih, "解包");
    add(ChooseDir, 4, Kind::Button, c(1), r(4), iw - 20, ih, "选择目录");
    add(Dir, 4, Kind::Input, c(2) - 20, r(4), iw * 2 + m + 65, ih);
    add(Pack, 4, Kind::Button, c(4) + 45, r(4), iw - 45, ih, "打包");
    add(Debug, 4, Kind::Choice, c(1), r(5), iw - 30, ih);
    add(SetDebug, 4, Kind::Button, c(2) - 30, r(5), iw - 40, ih, "调试");
    add(Speed, 4, Kind::Choice, c(3) - 60, r(5), iw - 30, ih);
    add(SetSpeed, 4, Kind::Button, c(4) - 90, r(5), iw - 40, ih, "速度");
    flag(Flag::Limbo, 4, c(4) - 15, r(5), iw + 15, "显示隐藏页面");
    add(Language, 4, Kind::Choice, c(1), r(6), iw - 15, ih, "中文 / English");
    add(Tooltips, 4, Kind::Check, c(2) - 15, r(6), iw + 35, ih,
        "English Tooltips");
    add(Documentation, 4, Kind::Button, c(3) + 30, r(6), iw - 15, ih, "文档");
    add(About, 4, Kind::Button, c(4) + 15, r(6), iw - 15, ih, "关于 ...");
  } else {
    // English reference has seven resource rows and a slightly different grid.
    add(-3, 0, Kind::Label, c(1), r(1), iw - 40, ih, "Sunlight");
    add(SunInput, 0, Kind::Input, c(1) + iw - 40 + m, r(1), 85 - m, ih);
    add(SetSun, 0, Kind::Button, c(2) + 45, r(1), iw - 50, ih, "Value");
    add(-1, 0, Kind::Label, c(1), r(2), iw - 40, ih, "Wallet (x10)");
    add(MoneyInput, 0, Kind::Input, c(1) + iw - 40 + m, r(2), 85 - m, ih);
    add(SetMoney, 0, Kind::Button, c(2) + 45, r(2), iw - 50, ih, "Coins");
    add(-2, 0, Kind::Label, c(1), r(3), iw - 40, ih, "Wisdom Tree");
    add(TreeInput, 0, Kind::Input, c(1) + iw - 40 + m, r(3), 85 - m, ih);
    add(SetTree, 0, Kind::Button, c(2) + 45, r(3), iw - 50, ih, "Feet");
    flag(Flag::SunLimit, 0, c(3), r(1), iw * 2 + m - 75,
         "No Upper Limit to Sunlight");
    flag(Flag::Fertilizer, 0, c(3), r(2), iw, "Infinite Fertilizer");
    flag(Flag::BugSpray, 0, c(4), r(2), iw, "Infinite Bug Spray");
    flag(Flag::TreeFood, 0, c(3), r(3), iw, "Infinite Tree Food");
    flag(Flag::Chocolate, 0, c(4), r(3), iw, "Infinite Chocolate");
    flag(Flag::AutoCollect, 0, c(1), r(4), iw, "Auto Collected");
    flag(Flag::NoLoot, 0, c(2), r(4), iw, "Don't Drop Loot");
    flag(Flag::FreePlanting, 0, c(3), r(4), iw, "Free Planting");
    flag(Flag::Anywhere, 0, c(4), r(4), iw, "Place Anywhere");
    add(Unlock, 0, Kind::Button, c(1), r(5), iw * 2 + m - 70, ih,
        "Get Gold Sunflower Trophy");
    flag(Flag::FastBelt, 0, c(3) - 58, r(5), iw + 40, "Seamless Conveyor Belt");
    flag(Flag::LockShovel, 0, c(4) - 18, r(5), iw + 18, "Shovel Continuously");
    add(Mode, 0, Kind::Choice, c(1), r(6), iw * 2 + m + 20, ih);
    add(Adventure, 0, Kind::Choice, c(3) + 20, r(6), iw - 20, ih);
    add(Mix, 0, Kind::Button, c(4), r(6), iw, ih, "Modify Mode");
    add(Win, 0, Kind::Button, c(1), r(7), iw - 5, ih, "Level Complete");
    add(BrightestCob, 0, Kind::Check, c(2) - 5, r(7), iw + 25, ih,
        "End with Brightest Cob");
    add(Rounds, 0, Kind::Input, c(3) + 20, r(7), iw - 20, ih);
    add(SetRounds, 0, Kind::Button, c(4), r(7), iw, ih, "Endless Rounds");

    auto br = [=](int row) { return r(row) + row * 4 + 2; };
    add(Row, 1, Kind::Choice, c(1), br(1), iw - 10, ih);
    add(Col, 1, Kind::Choice, c(1), br(2), iw - 10, ih);
    add(Plant, 1, Kind::Choice, c(2) - 10, br(1), iw + 80, ih);
    add(Zombie, 1, Kind::Choice, c(2) - 10, br(2), iw + 80, ih);
    add(PutPlant, 1, Kind::Button, c(3) + 70, br(1), iw - 35, ih, "Put Plant");
    add(Imitater, 1, Kind::Check, c(4) + 35, br(1), iw - 35, ih, "Imitater");
    add(PutZombie, 1, Kind::Button, c(3) + 70, br(2), iw - 35, ih,
        "Put Zombie");
    add(Grave, 1, Kind::Button, c(4) + 35, br(2), iw - 35, ih, "Put Grave");
    add(Ladder, 1, Kind::Button, c(3) + 70, br(3), iw - 35, ih, "Put Ladder");
    add(Rake, 1, Kind::Button, c(4) + 35, br(3), iw - 35, ih, "Put Rake");
    add(Mowers, 1, Kind::Choice, c(1), br(3), iw - 10, ih, "Lawn Mower");
    add(ClearType, 1, Kind::Choice, c(2) - 10, br(3), 100, ih);
    add(Clear, 1, Kind::Button, c(2) - 10 + 100 + m, br(3), iw + 80 - 100 - m,
        ih, "Clear All");
    flag(Flag::PlantInvincible, 1, c(1), br(4), iw + 5, "Plant Invincible");
    flag(Flag::PlantWeak, 1, c(2) + 5, br(4), iw - 20, "Plant Weak");
    flag(Flag::Reload, 1, c(1), br(5), iw + 5, "Reload Instantly");
    flag(Flag::Butter, 1, c(2) + 5, br(5), iw - 20, "Always Butter");
    flag(Flag::Awake, 1, c(1), br(6), iw + 5, "Mushroom Awake");
    flag(Flag::NoCrater, 1, c(2) + 5, br(6), iw - 20, "No Crater");
    flag(Flag::ZombieInvincible, 1, c(3), br(4), iw + 8, "Zombie Invincible");
    flag(Flag::ZombieWeak, 1, c(4) + 8, br(4), iw - 8, "Zombie Weak");
    flag(Flag::StopSpawning, 1, c(3), br(5), iw + 8, "Pause Spawning");
    flag(Flag::StopZombies, 1, c(4) + 8, br(5), iw - 8, "Stay in Place");
    flag(Flag::NoIce, 1, c(3), br(6), iw + 8, "No Ice Trail");
    flag(Flag::NoExplode, 1, c(4) + 8, br(6), iw - 8, "Never Explode");

    add(Slot, 2, Kind::Choice, c(1), r(1), iw - 10, ih);
    add(Seed, 2, Kind::Choice, c(2) - 10, r(1), iw + 80, ih);
    add(SetSlot, 2, Kind::Button, c(3) + 70, r(1), iw - 15, ih, "Replace Seed");
    add(SeedImitater, 2, Kind::Check, c(4) + 55, r(1), iw - 55, ih, "Imitater");
    add(Quick, 2, Kind::Check, c(1), r(2), iw - 10, ih, "Lineup Mode");
    add(AutoLadder, 2, Kind::Button, c(2) - 10, r(2), iw - 10, ih,
        "Smart Laddering");
    add(Lily, 2, Kind::Choice, c(3) - 20, r(2), iw + 5, ih, "Pool's Lily Pad");
    add(Pot, 2, Kind::Choice, c(4) - 15, r(2), iw + 15, ih,
        "Roof's Flower Pot");
    add(ResetScene, 2, Kind::Button, c(1), r(3), iw - 10, ih, "Reset Scene");
    add(Scene, 2, Kind::Choice, c(2) - 10, r(3), iw - 10, ih);
    add(LoadLineup, 2, Kind::Button, c(3) - 20, r(3), iw * 2 + m + 20, ih,
        "Load Lineup List File (***.yml)");
    add(LineupFile, 2, Kind::Choice, c(3) - 20, r(3), iw * 2 + m + 20, ih);
    add(LineupText, 2, Kind::Text, c(1), r(4), iw * 4 + m * 3, ih * 2 + m);
    add(GetLineup, 2, Kind::Button, c(1), r(6), iw + 60, ih, "Get Lineup Code");
    add(CopyLineup, 2, Kind::Button, c(1), r(7), iw + 60, ih, "Copy (Export)");
    add(ApplyLineup, 2, Kind::Button, c(4) - 60, r(6), iw + 60, ih,
        "Apply Current Setup");
    add(PasteLineup, 2, Kind::Button, c(4) - 60, r(7), iw + 60, ih,
        "Paste (Import)");
    add(Screenshot, 2, Kind::Button, c(2) + 70, r(6), iw * 2 + m - 140,
        ih * 2 + m, "Screenshot\nto\nClipboard");

    // English reference uses 5x4 textual zombie checks.
    constexpr const char *labels[20] = {"Conehead ~",
                                        "Pole Vaulting ~",
                                        "Buckethead ~",
                                        "Newspaper ~",
                                        "Screen Door ~",
                                        "Football ~",
                                        "Dancing ~",
                                        "Snorkel ~",
                                        "Zomboni",
                                        "Dolphin Rider ~",
                                        "Jack-in-the-Box ~",
                                        "Balloon ~",
                                        "Digger ~",
                                        "Pogo ~",
                                        "~ Yeti",
                                        "Bungee ~",
                                        "Ladder ~",
                                        "Catapult ~",
                                        "Gargantuar",
                                        "GigaGargantuar"};
    for (int row = 0; row < 5; ++row)
      for (int col = 0; col < 4; ++col) {
        const int i = row * 4 + col;
        add(SpawnFirst + i, 3, Kind::Check, c(col + 1), r(row + 1), iw, ih,
            labels[i]);
      }
    // FLTK places this choice's label to its left; encode that visible label.
    add(-4, 3, Kind::Label, c(1), r(6), c(3) - c(1) - 8, ih,
        "GigaGargantuar's Weight in Non-huge Waves:");
    add(GigaWeight, 3, Kind::Choice, c(3), r(6), iw - 48, ih);
    add(GigaLimit, 3, Kind::Check, c(1), r(7), iw * 2 + m, ih,
        "No GigaGargantuar in 11th ~ 19th Waves");
    add(Details, 3, Kind::Button, c(4), r(6), iw, ih, "Show Details");
    add(Generate, 3, Kind::Button, c(4) - 80, r(7), iw + 80, ih,
        "Extreme (Filling Evenly)");
    add(SpawnMode, 3, Kind::Choice, c(4) - 80, r(7), iw + 80, ih,
        "Select Spawning Mode:");

    add(Music, 4, Kind::Choice, c(1), r(1), iw + 50, ih);
    add(PlayMusic, 4, Kind::Button, c(2) + 50, r(1), iw + 15, ih,
        "Background Music");
    add(SaveFolder, 4, Kind::Button, c(3) + 65, r(1), iw * 2 + m - 65, ih,
        "Open Userdata Folder");
    flag(Flag::NoFog, 4, c(1), r(2), iw - 10, "Clear Fog");
    flag(Flag::SeeVase, 4, c(2) - 10, r(2), iw, "Transparent Vase");
    flag(Flag::Readonly, 4, c(4) - 15, r(2), iw + 15, "Userdata Readonly");
    add(ChooseFile, 4, Kind::Button, c(1), r(3), iw - 10, ih, "Choose File");
    add(File, 4, Kind::Input, c(2) - 10, r(3), iw * 2 + m + 55, ih);
    add(Unpack, 4, Kind::Button, c(4) + 45, r(3), iw - 45, ih, "Extract");
    add(ChooseDir, 4, Kind::Button, c(1), r(4), iw - 10, ih, "Choose Folder");
    add(Dir, 4, Kind::Input, c(2) - 10, r(4), iw * 2 + m + 55, ih);
    add(Pack, 4, Kind::Button, c(4) + 45, r(4), iw - 45, ih, "Pack");
    add(Debug, 4, Kind::Choice, c(1), r(5), iw - 10, ih);
    add(SetDebug, 4, Kind::Button, c(2) - 10, r(5), iw - 30, ih, "Debug");
    add(Speed, 4, Kind::Choice, c(1), r(6), iw - 10, ih);
    add(SetSpeed, 4, Kind::Button, c(2) - 10, r(6), iw - 30, ih, "Speed");
    flag(Flag::Limbo, 4, c(3), r(5), iw + 5, "Show Limbo Page");
    flag(Flag::Background, 4, c(3), r(6), iw * 2 + m - 40,
         "Enable PvZ to Run in Background");
    add(Language, 4, Kind::Choice, c(1), r(7), iw - 10, ih, "中文 / English");
    add(Documentation, 4, Kind::Button, c(3), r(7), iw, ih, "Document");
    add(About, 4, Kind::Button, c(4), r(7), iw, ih, "About ...");
  }
  return result;
}
} // namespace Trainer::Classic
