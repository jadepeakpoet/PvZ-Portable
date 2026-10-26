// SPDX-License-Identifier: LGPL-3.0-or-later
#include "TrainerFormats.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <zlib.h>
namespace Trainer::Formats {
namespace {
constexpr char alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::string Base64(const std::vector<unsigned char> &bytes) {
  std::string out;
  unsigned bits = 0;
  int count = 0;
  for (auto c : bytes) {
    bits = (bits << 8) | c;
    count += 8;
    while (count >= 6) {
      count -= 6;
      out += alphabet[(bits >> count) & 63];
    }
  }
  if (count)
    out += alphabet[(bits << (6 - count)) & 63];
  while (out.size() % 4)
    out += '=';
  return out;
}
std::vector<unsigned char> Unbase64(const std::string &text) {
  if (text.empty() || text.size() > 256 || text.size() % 4)
    throw std::runtime_error("Invalid lineup code length");
  std::vector<unsigned char> out;
  unsigned bits = 0;
  int count = 0;
  bool padding = false;
  auto firstPad = text.find('=');
  if (firstPad != std::string::npos && text.size() - firstPad > 2)
    throw std::runtime_error("Invalid base64 padding");
  for (char c : text) {
    if (c == '=') {
      padding = true;
      continue;
    }
    if (padding)
      throw std::runtime_error("Invalid base64 padding");
    if (c == '-')
      c = '+';
    if (c == '_')
      c = '/';
    const auto *p = std::find(alphabet, alphabet + 64, c);
    if (p == alphabet + 64)
      throw std::runtime_error("Invalid lineup code alphabet");
    bits = (bits << 6) | unsigned(p - alphabet);
    count += 6;
    if (count >= 8) {
      count -= 8;
      out.push_back((bits >> count) & 255);
    }
  }
  if (count && (bits & ((1U << count) - 1)))
    throw std::runtime_error("Invalid base64 padding");
  if (out.size() !=
      text.size() / 4 * 3 -
          (firstPad == std::string::npos ? 0 : text.size() - firstPad))
    throw std::runtime_error("Invalid base64 padding");
  return out;
}
void Validate(const Lineup &l) {
  if (l.scene < 0 || l.scene > 5 || l.rakeRow < 0 ||
      l.rakeRow > ((l.scene == 2 || l.scene == 3) ? 6 : 5))
    throw std::runtime_error("Invalid lineup scene/rake row");
  if ((l.scene == 2 || l.scene == 3) && (l.rakeRow == 3 || l.rakeRow == 4))
    throw std::runtime_error("Rake cannot be in pool");
  for (auto cell : l.cells)
    if ((cell >> 10) > 48)
      throw std::runtime_error("Invalid lineup plant ID");
}
void WriteLE(std::ostream &out, std::uint64_t value, int width) {
  for (int i = 0; i < width; ++i)
    out.put(char((value >> (i * 8)) & 255));
}
std::uint64_t ReadLE(std::istream &in, int width) {
  std::uint64_t n = 0;
  for (int i = 0; i < width; ++i) {
    int c = in.get();
    if (c < 0)
      throw std::runtime_error("Truncated PAK");
    n |= std::uint64_t(c) << (i * 8);
  }
  return n;
}
std::string FileBytes(const std::filesystem::path &p) {
  std::ifstream f(p, std::ios::binary);
  if (!f)
    throw std::runtime_error("Cannot read file");
  return {std::istreambuf_iterator<char>(f), {}};
}
} // namespace
std::string EncodeLineup(const Lineup &l) {
  Validate(l);
  int count = (l.scene == 2 || l.scene == 3) ? 54 : 45;
  std::array<unsigned char, 108> raw{};
  for (int i = 0; i < count; ++i) {
    raw[2 * i] = l.cells[i] & 255;
    raw[2 * i + 1] = l.cells[i] >> 8;
  }
  uLongf size = compressBound(count * 2);
  std::vector<unsigned char> encoded(size);
  if (compress2(encoded.data(), &size, raw.data(), count * 2,
                Z_BEST_COMPRESSION) != Z_OK)
    throw std::runtime_error("Lineup compression failed");
  encoded.resize(size);
  encoded.push_back((l.rakeRow << 4) | l.scene);
  for (auto &c : encoded)
    c ^= 0x54;
  return Base64(encoded);
}
Lineup DecodeLineup(const std::string &text) {
  // Public textual lineup format uses hexadecimal object IDs and 1-based cells.
  if (text.size() == 1 || text.find(',') != std::string::npos) {
    if (text.size() > 32768)
      throw std::runtime_error("Invalid lineup code length");
    std::istringstream input(text);
    std::string item;
    std::getline(input, item, ',');
    std::size_t used = 0;
    int scene = std::stoi(item, &used);
    if (used != item.size() || scene < 0 || scene > 5)
      throw std::runtime_error("Invalid lineup scene/rake row");
    constexpr int scenes[] = {2, 3, 0, 1, 4, 5};
    Lineup l;
    l.scene = scenes[scene];
    int entries = 0;
    while (std::getline(input, item, ',')) {
      if (++entries > 500)
        throw std::runtime_error("Invalid lineup code length");
      std::istringstream row(item);
      std::string id;
      int y = 0, x = 0, state = 0, unused = 0, imitater = 0;
      if (!(row >> id >> y >> x >> state >> unused))
        throw std::runtime_error("Invalid lineup compressed data");
      int type = std::stoi(id, &used, 16);
      if (used != id.size() || type < 0 || type > 50 || y < 1 ||
          y > ((l.scene == 2 || l.scene == 3) ? 6 : 5) || x < 1 || x > 9 ||
          state < 0 || state > 2 || unused < 0 || unused > 4)
        throw std::runtime_error("Invalid lineup plant ID");
      if (row >> imitater) {
        if (imitater != 0 && imitater != 1)
          throw std::runtime_error("Invalid lineup plant ID");
      }
      auto &cell = l.cells[(y - 1) * 9 + x - 1];
      if (type == 48)
        cell |= 1;
      else if (type == 49)
        l.rakeRow = y;
      else if (type == 50)
        cell = (cell & ~192) | 192;
      else if (type == 16 || type == 33)
        cell =
            (cell & ~224) | ((type == 16 ? 1 : 2) << 6) | (imitater ? 32 : 0);
      else if (type == 30)
        cell |= 16 | (imitater ? 8 : 0);
      else if (type == 35)
        cell |= 4 | (imitater ? 2 : 0);
      else {
        bool mushroom = type == 8 || type == 9 || type == 10 || type == 12 ||
                        type == 13 || type == 14 || type == 15 || type == 24 ||
                        type == 31 || type == 42;
        bool awake = !mushroom || state != 0 || l.scene == 1 || l.scene == 3 ||
                     l.scene == 5;
        cell = (cell & 255) | ((type + 1) << 10) | (imitater ? 512 : 0) |
               (awake ? 256 : 0);
      }
    }
    Validate(l);
    return l;
  }
  auto bytes = Unbase64(text);
  if (bytes.size() < 3)
    throw std::runtime_error("Truncated lineup");
  for (auto &c : bytes)
    c ^= 0x54;
  Lineup l;
  l.scene = bytes.back() & 15;
  l.rakeRow = bytes.back() >> 4;
  bytes.pop_back();
  Validate(l);
  std::array<unsigned char, 108> raw{};
  uLongf len = raw.size();
  if (uncompress(raw.data(), &len, bytes.data(), bytes.size()) != Z_OK ||
      len != std::size_t((l.scene == 2 || l.scene == 3) ? 108 : 90))
    throw std::runtime_error("Invalid lineup compressed data");
  for (std::size_t i = 0; i < len / 2; ++i)
    l.cells[i] = raw[i * 2] | (unsigned(raw[i * 2 + 1]) << 8);
  Validate(l);
  return l;
}
std::vector<int> ParseIntegers(const std::string &text, int low, int high,
                               std::size_t limit) {
  std::string normalized = text;
  for (auto &c : normalized)
    if (c == ',' || c == ';' || c == '[' || c == ']')
      c = ' ';
  std::istringstream in(normalized);
  std::vector<int> out;
  std::string token;
  while (in >> token) {
    std::size_t used = 0;
    long long n = std::stoll(token, &used);
    if (used != token.size() || n < low || n > high || out.size() >= limit)
      throw std::runtime_error("Invalid integer list");
    out.push_back(int(n));
  }
  return out;
}
void Pack(const std::filesystem::path &source,
          const std::filesystem::path &destination) {
  if (!std::filesystem::is_directory(source))
    throw std::runtime_error("PAK source must be directory");
  if (std::filesystem::exists(destination))
    throw std::runtime_error("Destination already exists");
  std::vector<std::filesystem::path> entries;
  std::uint64_t total = 0;
  for (auto &e : std::filesystem::recursive_directory_iterator(source)) {
    if (e.is_symlink())
      throw std::runtime_error("Symlinks are not supported");
    if (e.is_regular_file()) {
      auto rel = std::filesystem::relative(e.path(), source);
      auto name = rel.generic_string();
      auto size = e.file_size();
      if (name.size() > 255 ||
          size > std::numeric_limits<std::uint32_t>::max() ||
          (total += size) > 1024ULL * 1024 * 1024)
        throw std::runtime_error("PAK exceeds size limits");
      entries.push_back(rel);
    }
  }
  std::sort(entries.begin(), entries.end());
  std::ostringstream raw(std::ios::binary);
  WriteLE(raw, 0xBAC04AC0, 4);
  WriteLE(raw, 0, 4);
  for (auto &e : entries) {
    std::string name = e.generic_string();
    std::replace(name.begin(), name.end(), '/', '\\');
    raw.put(0);
    raw.put(char(name.size()));
    raw.write(name.data(), name.size());
    WriteLE(raw, std::filesystem::file_size(source / e), 4);
    WriteLE(raw, 0, 8);
  }
  raw.put(char(0x80));
  for (auto &e : entries) {
    auto content = FileBytes(source / e);
    raw.write(content.data(), content.size());
  }
  auto bytes = raw.str();
  for (auto &c : bytes)
    c = char(static_cast<unsigned char>(c) ^ 0xF7);
  std::ofstream out(destination, std::ios::binary);
  out.write(bytes.data(), bytes.size());
  if (!out)
    throw std::runtime_error("Cannot write PAK");
}
void Unpack(const std::filesystem::path &source,
            const std::filesystem::path &destination) {
  if (std::filesystem::file_size(source) > 1024ULL * 1024 * 1024)
    throw std::runtime_error("PAK exceeds size limits");
  if (std::filesystem::exists(destination))
    throw std::runtime_error("Destination already exists");
  auto bytes = FileBytes(source);
  for (auto &c : bytes)
    c = char(static_cast<unsigned char>(c) ^ 0xF7);
  std::istringstream in(bytes, std::ios::binary);
  if (ReadLE(in, 4) != 0xBAC04AC0 || ReadLE(in, 4) != 0)
    throw std::runtime_error("Unsupported PAK header");
  struct Entry {
    std::filesystem::path path;
    std::uint32_t size;
  };
  std::vector<Entry> entries;
  std::uint64_t sum = 0;
  while (!(ReadLE(in, 1) & 0x80)) {
    std::size_t len = ReadLE(in, 1);
    std::string name(len, '\0');
    in.read(name.data(), len);
    if (!in)
      throw std::runtime_error("Truncated PAK path");
    std::replace(name.begin(), name.end(), '\\', '/');
    std::filesystem::path p(name);
    if (p.empty() || p.is_absolute() || name.find(':') != std::string::npos ||
        name.find('\0') != std::string::npos)
      throw std::runtime_error("Unsafe PAK path");
    for (const auto &c : p)
      if (c == ".." || c == ".")
        throw std::runtime_error("Unsafe PAK path");
    auto size = ReadLE(in, 4);
    ReadLE(in, 8);
    sum += size;
    entries.push_back({p, std::uint32_t(size)});
    if (entries.size() > 100000)
      throw std::runtime_error("Too many PAK entries");
  }
  auto offset = in.tellg();
  if (offset < 0 || sum != bytes.size() - std::size_t(offset))
    throw std::runtime_error("Invalid PAK data length");
  std::filesystem::create_directories(destination);
  for (auto &e : entries) {
    auto path = destination / e.path;
    if (std::filesystem::exists(path))
      throw std::runtime_error("Duplicate PAK path");
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    std::string content(e.size, '\0');
    in.read(content.data(), e.size);
    out.write(content.data(), content.size());
    if (!out)
      throw std::runtime_error("Cannot extract PAK file");
  }
}
} // namespace Trainer::Formats
