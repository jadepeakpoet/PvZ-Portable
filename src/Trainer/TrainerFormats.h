// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
namespace Trainer::Formats {
struct Lineup {
  int scene = 2;
  int rakeRow = 0;
  std::array<std::uint16_t, 54> cells{};
};
std::string EncodeLineup(const Lineup &);
Lineup DecodeLineup(const std::string &);
std::vector<int> ParseIntegers(const std::string &, int low, int high,
                               std::size_t limit);
void Pack(const std::filesystem::path &source,
          const std::filesystem::path &destination);
void Unpack(const std::filesystem::path &source,
            const std::filesystem::path &destination);
} // namespace Trainer::Formats
