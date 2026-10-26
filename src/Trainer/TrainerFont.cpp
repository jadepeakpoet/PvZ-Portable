// SPDX-License-Identifier: LGPL-3.0-or-later
#include "TrainerFont.h"
#include "Common.h"
#include "Trainer.h"
#include "TrainerNames.h"
#include "graphics/Graphics.h"
#include "graphics/MemoryImage.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "third_party/stb_truetype.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <unordered_map>
#include <vector>
#if defined(__APPLE__)
#include <CoreText/CoreText.h>
#endif

namespace Trainer {
namespace {
struct FontData {
  std::vector<unsigned char> bytes;
  stbtt_fontinfo info{};
  std::string source;
  bool chinese = false;
};

// stb_truetype expects trusted font data. Validate the sfnt table envelope as
// an additional guard against truncated files; this is not an untrusted-font
// parser.
bool ValidEnvelope(const std::vector<unsigned char> &b, int offset) {
  if (offset < 0 || b.size() < 12 ||
      static_cast<size_t>(offset) > b.size() - 12)
    return false;
  auto u16 = [&](size_t p) { return unsigned(b[p]) * 256 + b[p + 1]; };
  auto u32 = [&](size_t p) {
    return (uint32_t(b[p]) << 24) | (uint32_t(b[p + 1]) << 16) |
           (uint32_t(b[p + 2]) << 8) | uint32_t(b[p + 3]);
  };
  size_t count = u16(offset + 4), start = size_t(offset) + 12;
  if (count == 0 || count > (b.size() - start) / 16)
    return false;
  for (size_t i = 0; i < count; ++i) {
    size_t entry = start + i * 16;
    size_t table = u32(entry + 8), length = u32(entry + 12);
    if (table > b.size() || length > b.size() - table)
      return false;
  }
  return true;
}

std::shared_ptr<FontData> LoadFont(const std::filesystem::path &path) {
  std::error_code ec;
  auto size = std::filesystem::file_size(path, ec);
  if (ec || size < 12 || size > 128 * 1024 * 1024)
    return {};
  std::ifstream file(path, std::ios::binary);
  if (!file)
    return {};
  auto data = std::make_shared<FontData>();
  data->bytes.resize(static_cast<size_t>(size));
  if (!file.read(reinterpret_cast<char *>(data->bytes.data()),
                 data->bytes.size()))
    return {};
  const auto &bytes = data->bytes;
  std::vector<int> offsets{0};
  // Inspect collection faces: the first one may cover traditional Chinese but
  // omit simplified labels. Validate the actual catalog instead of three
  // glyphs.
  if (bytes[0] == 't' && bytes[1] == 't' && bytes[2] == 'c' &&
      bytes[3] == 'f') {
    if (bytes.size() < 16)
      return {};
    auto u32 = [&](size_t i) {
      return (uint32_t(bytes[i]) << 24) | (uint32_t(bytes[i + 1]) << 16) |
             (uint32_t(bytes[i + 2]) << 8) | bytes[i + 3];
    };
    auto count = u32(8);
    if (!count || count > 64 || count > (bytes.size() - 12) / 4)
      return {};
    offsets.clear();
    for (unsigned i = 0; i < count; ++i) {
      auto n = u32(12 + 4 * i);
      if (n <= uint32_t(std::numeric_limits<int>::max()))
        offsets.push_back(static_cast<int>(n));
    }
  }
  static const auto required = [] {
    std::vector<char32_t> characters;
    auto add = [&](std::string_view text) {
      size_t pos = 0;
      char32_t cp;
      while (Sexy::UTF8DecodeNext(text, pos, cp))
        if (cp >= 0x2e80)
          characters.push_back(cp);
    };
    for (auto &feature : Features()) {
      add(feature.zh);
      add(feature.hintZh);
    }
    for (auto name : PlantNames)
      add(name.second);
    for (auto name : ZombieNames)
      add(name.second);
    for (auto name : ModeNames)
      add(name.second);
    add("资源战场阵型出怪杂项关闭工具箱类型列表完成选择数值附加文本文件阳光金币"
        "暂停继续应用");
    std::sort(characters.begin(), characters.end());
    characters.erase(std::unique(characters.begin(), characters.end()),
                     characters.end());
    return characters;
  }();
  int best = -1;
  for (int offset : offsets) {
    stbtt_fontinfo info{};
    if (!ValidEnvelope(bytes, offset) ||
        !stbtt_InitFont(&info, bytes.data(), offset) ||
        !stbtt_FindGlyphIndex(&info, 'A'))
      continue;
    int score = 0;
    for (auto cp : required)
      score += stbtt_FindGlyphIndex(&info, cp) != 0;
    if (score > best) {
      best = score;
      data->info = info;
    }
    if (score == static_cast<int>(required.size()))
      break;
  }
  if (best < 0)
    return {};
  data->chinese = best == static_cast<int>(required.size());
  data->source = path.string();
  return data;
}

std::vector<std::filesystem::path> Candidates() {
  std::vector<std::filesystem::path> paths{
      std::filesystem::u8path(Sexy::GetResourcePath("trainer-font.ttf")),
      std::filesystem::u8path(Sexy::GetResourcePath("trainer-font.otf")),
      std::filesystem::u8path(Sexy::GetResourcePath("trainer-font.ttc"))};
#if defined(__APPLE__)
  // Ask CoreText for the installed CJK fallback rather than relying solely on
  // private/versioned font paths, especially on iOS.
  CTFontRef base =
      CTFontCreateWithName(CFSTR("PingFangSC-Regular"), 17, nullptr);
  if (base) {
    CTFontRef cjk =
        CTFontCreateForString(base, CFSTR("阳光僵尸"), CFRangeMake(0, 4));
    if (cjk) {
      CFURLRef url =
          static_cast<CFURLRef>(CTFontCopyAttribute(cjk, kCTFontURLAttribute));
      UInt8 path[4096]{};
      if (url && CFGetTypeID(url) == CFURLGetTypeID() &&
          CFURLGetFileSystemRepresentation(url, true, path, sizeof(path)))
        paths.emplace_back(reinterpret_cast<const char *>(path));
      if (url)
        CFRelease(url);
      CFRelease(cjk);
    }
    CFRelease(base);
  }
#endif
#if defined(_WIN32)
  const char *windows = std::getenv("WINDIR");
  std::filesystem::path fonts =
      std::filesystem::path(windows ? windows : "C:/Windows") / "Fonts";
  for (const char *name :
       {"msyh.ttc", "msyh.ttf", "simhei.ttf", "simsun.ttc", "arial.ttf"})
    paths.push_back(fonts / name);
#elif defined(__ANDROID__)
  for (const char *path : {"/system/fonts/NotoSansCJK-Regular.ttc",
                           "/system/fonts/NotoSansSC-Regular.otf",
                           "/system/fonts/DroidSansFallback.ttf",
                           "/system/fonts/Roboto-Regular.ttf"})
    paths.emplace_back(path);
#elif defined(__APPLE__)
  for (const char *path :
       {"/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/LanguageSupport/PingFang.ttc",
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/Library/Fonts/Arial Unicode.ttf",
        "/System/Library/Fonts/Supplemental/Songti.ttc",
        "/System/Library/Fonts/STHeiti Light.ttc",
        "/System/Library/Fonts/Cache/STHeiti-Light.ttc",
        "/System/Library/Fonts/Helvetica.ttc"})
    paths.emplace_back(path);
#else
  for (const char *path :
       {"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/google-noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf"})
    paths.emplace_back(path);
#endif
  return paths;
}

std::shared_ptr<FontData> FindFont() {
  std::shared_ptr<FontData> latin;
  for (const auto &path : Candidates()) {
    auto font = LoadFont(path);
    if (!font)
      continue;
    if (font->chinese)
      return font;
    if (!latin)
      latin = std::move(font);
  }
#if defined(__APPLE__)
  // Recent macOS versions move PingFang into versioned system asset folders.
  // Scan only that shallow location, never the user's filesystem.
  std::error_code ec;
  std::filesystem::directory_iterator roots("/System/Library/AssetsV2", ec);
  if (!ec)
    for (const auto &root : roots) {
      if (!root.path().filename().string().starts_with(
              "com_apple_MobileAsset_Font"))
        continue;
      std::filesystem::directory_iterator assets(root.path(), ec);
      if (ec) {
        ec.clear();
        continue;
      }
      for (const auto &asset : assets) {
        auto font = LoadFont(asset.path() / "AssetData/PingFang.ttc");
        if (font && font->chinese)
          return font;
      }
    }
#endif
  return latin;
}

std::shared_ptr<FontData> Data() {
  static const std::shared_ptr<FontData> data = FindFont();
  return data;
}

class TrueTypeFont final : public Sexy::_Font {
  struct Glyph {
    int index = 0, advance = 0, x = 0, y = 0;
    std::unique_ptr<Sexy::MemoryImage> image;
  };
  std::shared_ptr<FontData> data;
  int height;
  float scale;
  std::unordered_map<char32_t, Glyph> glyphs;

  int GlyphIndex(char32_t cp) const {
    int index = stbtt_FindGlyphIndex(&data->info, static_cast<int>(cp));
    if (!index)
      index = stbtt_FindGlyphIndex(&data->info, 0xfffd);
    if (!index)
      index = stbtt_FindGlyphIndex(&data->info, '?');
    return index;
  }

  Glyph &GetGlyph(char32_t cp) {
    if (auto found = glyphs.find(cp); found != glyphs.end())
      return found->second;
    // Never evict an image mid-frame: the GL renderer may still have queued
    // draws referring to it. Bound arbitrary user text with a replacement.
    if (cp != U'?' && glyphs.size() >= 2047)
      return GetGlyph(U'?');
    auto &glyph = glyphs[cp];
    glyph.index = GlyphIndex(cp);
    int advance = 0, bearing = 0;
    stbtt_GetGlyphHMetrics(&data->info, glyph.index, &advance, &bearing);
    glyph.advance = static_cast<int>(std::lround(advance * scale));
    int width = 0, bitmapHeight = 0;
    unsigned char *bitmap =
        stbtt_GetGlyphBitmap(&data->info, scale, scale, glyph.index, &width,
                             &bitmapHeight, &glyph.x, &glyph.y);
    if (bitmap && width > 0 && bitmapHeight > 0) {
      glyph.image = std::make_unique<Sexy::MemoryImage>();
      glyph.image->Create(width, bitmapHeight);
      auto *pixels = glyph.image->GetBits();
      for (int i = 0; i < width * bitmapHeight; ++i)
        pixels[i] = (uint32_t(bitmap[i]) << 24) | 0x00ffffff;
      glyph.image->BitsChanged();
      glyph.image->SetImageMode(true, true);
    }
    stbtt_FreeBitmap(bitmap, data->info.userdata);
    return glyph;
  }

public:
  TrueTypeFont(std::shared_ptr<FontData> font, int pixels)
      : data(std::move(font)), height(std::clamp(pixels, 8, 96)),
        scale(stbtt_ScaleForPixelHeight(&data->info,
                                        static_cast<float>(height))) {
    int ascent = 0, descent = 0, gap = 0;
    stbtt_GetFontVMetrics(&data->info, &ascent, &descent, &gap);
    mAscent = static_cast<int>(std::ceil(ascent * scale));
    mHeight = std::max(height,
                       mAscent + static_cast<int>(std::ceil(-descent * scale)));
    mLineSpacingOffset =
        std::max(2, static_cast<int>(std::lround(gap * scale)));
  }

  int CharWidth(char32_t cp) override {
    int advance = 0, bearing = 0;
    stbtt_GetGlyphHMetrics(&data->info, GlyphIndex(cp), &advance, &bearing);
    return static_cast<int>(std::lround(advance * scale));
  }
  int CharWidthKern(char32_t cp, char32_t previous) override {
    int kern =
        previous ? static_cast<int>(std::lround(
                       stbtt_GetGlyphKernAdvance(
                           &data->info, GlyphIndex(previous), GlyphIndex(cp)) *
                       scale))
                 : 0;
    return CharWidth(cp) + kern;
  }
  int StringWidth(std::string_view text) override {
    size_t position = 0;
    char32_t cp = 0, previous = 0;
    int width = 0;
    while (Sexy::UTF8DecodeNext(text, position, cp)) {
      width += CharWidthKern(cp, previous);
      previous = cp;
    }
    return width;
  }
  void DrawString(Sexy::Graphics *g, int x, int y, std::string_view text,
                  const Sexy::Color &color, const Sexy::Rect &) override {
    if (!g)
      return;
    g->PushState();
    g->SetColor(color);
    g->SetColorizeImages(true);
    size_t position = 0;
    char32_t cp = 0;
    int previous = 0;
    while (Sexy::UTF8DecodeNext(text, position, cp)) {
      auto &glyph = GetGlyph(cp);
      if (previous)
        x += static_cast<int>(std::lround(
            stbtt_GetGlyphKernAdvance(&data->info, previous, glyph.index) *
            scale));
      if (glyph.image)
        g->DrawImage(glyph.image.get(), x + glyph.x, y + glyph.y);
      x += glyph.advance;
      previous = glyph.index;
    }
    g->PopState();
  }
  Sexy::_Font *Duplicate() override { return new TrueTypeFont(data, height); }
};
} // namespace

std::unique_ptr<Sexy::_Font> CreateFont(int pixelHeight) {
  auto data = Data();
  if (!data)
    return {};
  return std::make_unique<TrueTypeFont>(std::move(data), pixelHeight);
}
bool HasChineseFont() {
  auto data = Data();
  return data && data->chinese;
}
const std::string &FontSource() {
  static const std::string empty;
  auto data = Data();
  return data ? data->source : empty;
}
} // namespace Trainer
