// SPDX-License-Identifier: LGPL-3.0-or-later
#include "Trainer/Trainer.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>
int main() {
  try {
    std::array<bool, static_cast<int>(Trainer::Flag::Count)> flags{};
    std::array<bool, static_cast<int>(Trainer::Action::About) + 1> actions{};
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
