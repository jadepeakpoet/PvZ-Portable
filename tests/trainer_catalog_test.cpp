// SPDX-License-Identifier: LGPL-3.0-or-later
#include "ConstEnums.h"
#include "Trainer/Trainer.h"
#include "Trainer/TrainerNames.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>
int main() {
  try {
    std::array<bool, static_cast<int>(Trainer::Flag::Count)> flags{};
    std::array<bool, static_cast<int>(Trainer::Action::About) + 1> actions{};
    static_assert(Trainer::PlantNames.size() ==
                  static_cast<int>(SeedType::SEED_IMITATER));
    static_assert(Trainer::ZombieNames.size() ==
                  static_cast<int>(ZombieType::NUM_ZOMBIE_TYPES));
    static_assert(Trainer::ModeNames.size() ==
                  static_cast<int>(GameMode::NUM_GAME_MODES) - 1);
    for (const auto &[en, zh] : Trainer::ModeNames)
      if (std::string_view(en).empty() || std::string_view(zh).empty())
        throw std::runtime_error("Missing game mode translation");
    if (std::string_view(
            Trainer::ModeNames[GameMode::GAMEMODE_TREE_OF_WISDOM].first) !=
            "Tree of Wisdom" ||
        std::string_view(Trainer::ModeNames[GameMode::GAMEMODE_UPSELL].first) !=
            "Upsell")
      throw std::runtime_error("Game mode name ID alignment mismatch");
    for (const auto &[en, zh] : Trainer::PlantNames)
      if (std::string_view(en).empty() || std::string_view(zh).empty())
        throw std::runtime_error("Missing plant name translation");
    for (const auto &[en, zh] : Trainer::ZombieNames)
      if (std::string_view(en).empty() || std::string_view(zh).empty())
        throw std::runtime_error("Missing zombie name translation");
    if (std::string_view(Trainer::PlantNames[SeedType::SEED_COBCANNON].first) !=
            "Cob Cannon" ||
        std::string_view(
            Trainer::ZombieNames[ZombieType::ZOMBIE_REDEYE_GARGANTUAR]
                .second) != "红眼巨人僵尸")
      throw std::runtime_error("Display name ID alignment mismatch");
    for (const auto &feature : Trainer::Features()) {
      if (feature.page < 0 || feature.page > 4 ||
          std::string_view(feature.en).empty() ||
          std::string_view(feature.zh).empty() ||
          std::string_view(feature.hintEn).empty() ||
          std::string_view(feature.hintZh).empty())
        throw std::runtime_error("Missing page or translation");
      if (feature.flag >= 0) {
        if (feature.flag >= int(flags.size()) || flags[feature.flag])
          throw std::runtime_error("Duplicate or invalid flag");
        flags[feature.flag] = true;
      } else
        actions[int(feature.action)] = true;
    }
    for (bool included : flags)
      if (!included)
        throw std::runtime_error("Missing flag in UI catalog");
    for (bool included : actions)
      if (!included)
        throw std::runtime_error("Missing action in UI catalog");
    std::cout << "Trainer catalog covers every action and flag with "
                 "Chinese/English labels and hints\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
