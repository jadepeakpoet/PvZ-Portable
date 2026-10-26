// SPDX-License-Identifier: LGPL-3.0-or-later
// Coordinates below are independent measurements from PvZ Toolkit's classic
// Chinese/English layouts, not values copied from the implementation under test.
#include "Trainer/TrainerClassicLayout.h"
#include <array>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
using namespace Trainer::Classic;
void Require(bool ok, std::string message) {
  if (!ok) throw std::runtime_error(std::move(message));
}
const Control& Find(const std::vector<Control>& controls, int id) {
  for (const auto& c : controls) if (c.id == id) return c;
  throw std::runtime_error("Missing classic control " + std::to_string(id));
}
void Rect(const std::vector<Control>& controls, int id, int x, int y, int w, int h) {
  const auto& c = Find(controls, id);
  Require(c.x == x && c.y == y && c.w == w && c.h == h,
          "Reference rectangle mismatch: " + std::to_string(id));
}
bool Solid(Kind kind) {
  return kind == Kind::Button || kind == Kind::Input || kind == Kind::Choice || kind == Kind::Text;
}
bool Intersects(const Control& a, const Control& b) {
  return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}
void CheckLayout(bool chinese) {
  const auto controls = Layout(chinese);
  Require(!controls.empty(), "Empty classic layout");
  std::array<int, 5> pages{};
  std::set<std::pair<int, int>> ids;
  for (const auto& c : controls) {
    Require(c.page >= -1 && c.page <= 4, "Invalid page");
    Require(c.w > 0 && c.h > 0 && c.x >= 0 && c.y >= 0 &&
            c.x + c.w <= Width(chinese) && c.y + c.h <= Height(chinese),
            "Control outside reference canvas: " + std::to_string(c.id));
    if (c.page >= 0) ++pages[c.page];
    if (c.kind != Kind::Label)
      Require(ids.emplace(c.page, c.id).second, "Duplicate interactive control ID");
    Require(c.label != nullptr, "Null control label");
  }
  for (int page = 0; page < 5; ++page)
    Require(pages[page] >= 8, "Missing classic page content");
  for (std::size_t i = 0; i < controls.size(); ++i)
    for (std::size_t j = i + 1; j < controls.size(); ++j) {
      const auto& a = controls[i]; const auto& b = controls[j];
      if (a.page != b.page || !Solid(a.kind) || !Solid(b.kind)) continue;
      // The original replaces its Load button with a lineup-file selector in
      // the same rectangle; its spawning mode menu similarly replaces Generate.
      // These original alternative states are never shown together.
      if ((a.id == LoadLineup && b.id == LineupFile) ||
          (b.id == LoadLineup && a.id == LineupFile) ||
          (a.id == Generate && b.id == SpawnMode) ||
          (b.id == Generate && a.id == SpawnMode)) continue;
      Require(!Intersects(a, b), "Overlapping solid controls: " +
              std::to_string(a.id) + "/" + std::to_string(b.id));
    }
  if (chinese) {
    Rect(controls, SunInput, 65, 55, 80, 25);
    Rect(controls, SetSun, 155, 55, 50, 25);
    Rect(controls, MoneyInput, 65, 90, 80, 25);
    Rect(controls, Mode, 20, 195, 220, 25);
    Rect(controls, Plant, 115, 55, 145, 25);
    Rect(controls, PutPlant, 270, 55, 75, 25);
    Rect(controls, LineupText, 20, 160, 410, 62);
  } else {
    Rect(controls, SunInput, 104, 48, 77, 24);
    Rect(controls, SetSun, 189, 48, 70, 24);
    Rect(controls, Plant, 134, 54, 200, 24);
    Rect(controls, PutPlant, 342, 54, 85, 24);
    Rect(controls, LineupText, 16, 144, 504, 56);
  }
}
}
int main() {
  try {
    using namespace Trainer::Classic;
    Require(Width(true) == 450 && Height(true) == 275,
            "Chinese reference canvas changed");
    Require(Width(false) == 536 && Height(false) == 280,
            "English reference canvas changed");
    CheckLayout(true); CheckLayout(false);
    const auto zh = Layout(true), en = Layout(false);
    Require(std::string_view(Find(zh, SetSun).label) != std::string_view(Find(en, SetSun).label),
            "Language switch did not change control labels");
    Require(Find(zh, Plant).x != Find(en, Plant).x,
            "English layout must use its original wider geometry");
    std::cout << "Classic Toolkit reference geometry, five-page coverage, control IDs and bilingual layouts pass\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n'; return 1;
  }
}
