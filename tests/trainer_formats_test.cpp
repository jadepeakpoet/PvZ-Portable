// SPDX-License-Identifier: LGPL-3.0-or-later
#include "Trainer/TrainerFormats.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace Trainer::Formats;
namespace {
void Check(bool v, const char *message) {
  if (!v)
    throw std::runtime_error(message);
}
template <class F> void Reject(F f) {
  bool rejected = false;
  try {
    f();
  } catch (const std::exception &) {
    rejected = true;
  }
  Check(rejected, "Invalid input was accepted");
}
std::string Read(const std::filesystem::path &p) {
  std::ifstream in(p, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), {}};
}
void Write(const std::filesystem::path &p, const std::string &bytes) {
  std::ofstream out(p, std::ios::binary);
  out.write(bytes.data(), bytes.size());
}
std::string Pak(const std::string &name, const std::string &content) {
  std::string out;
  auto put = [&](std::uint64_t n, int count) {
    for (int i = 0; i < count; ++i)
      out += char(n >> (i * 8));
  };
  put(0xBAC04AC0, 4);
  put(0, 4);
  put(0, 1);
  put(name.size(), 1);
  out += name;
  put(content.size(), 4);
  put(0, 8);
  put(0x80, 1);
  out += content;
  for (auto &c : out)
    c = char(static_cast<unsigned char>(c) ^ 0xF7);
  return out;
}
} // namespace
int main() {
  try {
    // Compatibility fixture: Toolkit misc/setups.yml, exported lineup code for
    // 五角星无炮流.
    const std::string fixture =
        "LI43jH/kRlzjZrhRF3RUpjJeOFHFZEXTHtMe0tlawVVE7GI0efJc8npSWMdVLhV261Y=";
    auto lineup = DecodeLineup(fixture);
    Check(lineup.scene == 2 && lineup.cells[0] == 48384 &&
              lineup.cells[10] == 39184,
          "Toolkit fixture decode mismatch");
    Check(EncodeLineup(lineup) == fixture, "Toolkit fixture export mismatch");
    auto textual =
        DecodeLineup("2,0 1 1 0 0 0,8 2 1 0 0 1,10 2 1 0 0 0,30 2 1 0 0 0");
    Check(textual.scene == 0 && textual.cells[0] == 1280,
          "Textual lineup scene/plant mismatch");
    Check((textual.cells[9] >> 10) == 9 && (textual.cells[9] & 512) &&
              !(textual.cells[9] & 256) && (textual.cells[9] & 64) &&
              (textual.cells[9] & 1),
          "Textual lineup layered sleeping imitater mismatch");
    Reject([] { DecodeLineup("2,0 0 1 0 0 0"); });
    for (int scene = 0; scene < 6; ++scene) {
      Lineup l;
      l.scene = scene;
      int rows = scene == 2 || scene == 3 ? 6 : 5;
      for (int i = 0; i < rows * 9; ++i)
        l.cells[i] = std::uint16_t(((i % 48 + 1) << 10) | (i % 1024));
      auto copy = DecodeLineup(EncodeLineup(l));
      Check(copy.scene == scene && copy.cells == l.cells,
            "Lineup roundtrip lost layered plant data");
    }
    Reject([] { DecodeLineup("AAAA"); });
    Reject([&] { DecodeLineup(fixture + "===="); });
    Reject([&] {
      auto bad = fixture;
      bad[0] = '!';
      DecodeLineup(bad);
    });
    Reject([] {
      Lineup l;
      l.scene = 2;
      l.rakeRow = 3;
      EncodeLineup(l);
    });
    Reject([] {
      Lineup l;
      l.cells[0] = 63 << 10;
      EncodeLineup(l);
    });
    Check(ParseIntegers("[0, 2;32]", 0, 32, 3) == std::vector<int>({0, 2, 32}),
          "Integer list mismatch");
    Reject([] { ParseIntegers("0 33", 0, 32, 100); });
    Reject([] { ParseIntegers("1 2 3", 0, 32, 2); });
    Reject([] { ParseIntegers("1junk", 0, 32, 2); });
    auto root =
        std::filesystem::temp_directory_path() /
        ("pvzp-formats-test-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root / "source/properties");
    Write(root / "source/properties/文本.xml", "<properties>中文</properties>");
    Write(root / "source/sound.bin", std::string("\0\xff\x12", 3));
    Pack(root / "source", root / "main.pak");
    Unpack(root / "main.pak", root / "extracted");
    Check(Read(root / "extracted/properties/文本.xml") ==
              Read(root / "source/properties/文本.xml"),
          "PAK unicode entry mismatch");
    Check(Read(root / "extracted/sound.bin") == Read(root / "source/sound.bin"),
          "PAK binary mismatch");
    Reject([&] { Pack(root / "source", root / "main.pak"); });
    Reject([&] { Unpack(root / "main.pak", root / "extracted"); });
    Write(root / "unsafe.pak", Pak("../escape.txt", "x"));
    Reject([&] { Unpack(root / "unsafe.pak", root / "unsafe-out"); });
    Check(!std::filesystem::exists(root / "escape.txt") &&
              !std::filesystem::exists(root / "unsafe-out"),
          "Unsafe PAK wrote outside destination");
    Write(root / "truncated.pak", Pak("ok.txt", "hello").substr(0, 12));
    Reject([&] { Unpack(root / "truncated.pak", root / "bad-out"); });
    std::filesystem::remove_all(root);
    std::cout
        << "Trainer format compatibility, validation and PAK tests passed\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
