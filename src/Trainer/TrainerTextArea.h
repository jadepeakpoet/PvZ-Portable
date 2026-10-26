// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include "Common.h"
#include "graphics/Font.h"
#include "widget/EditWidget.h"
#include <algorithm>
namespace Trainer {
using namespace Sexy;
class ClassicTextArea : public EditWidget {
  int topLine = 0;
  int LineHeight() const { return mFont ? mFont->GetHeight() + 2 : 18; }
  std::vector<std::pair<int, int>> Lines() const {
    std::vector<std::pair<int, int>> lines;
    int start = 0;
    for (int i = 0; i < int(mString.size());) {
      int next = static_cast<int>(UTF8NextBoundary(mString, i));
      if (mString[i] == '\n') {
        lines.emplace_back(start, i);
        start = next;
      } else if (mFont && i > start &&
                 mFont->StringWidth(std::string_view(mString).substr(
                     start, next - start)) > mWidth - 8) {
        lines.emplace_back(start, i);
        start = i;
      }
      i = next;
    }
    lines.emplace_back(start, int(mString.size()));
    return lines;
  }

public:
  ClassicTextArea(int id, EditListener *l) : EditWidget(id, l) {}
  void FocusCursor(bool) override {
    mLeftPos = 0;
    MarkDirty();
  }
  void KeyDown(KeyCode key) override {
    if (key == KEYCODE_RETURN) {
      InsertTextAtCursor("\n");
      MarkDirty();
      return;
    }
    if (key == KEYCODE_UP || key == KEYCODE_DOWN) {
      auto lines = Lines();
      int row = 0;
      for (int i = 0; i < int(lines.size()); ++i)
        if (mCursorPos >= lines[i].first)
          row = i;
      int target = std::clamp(row + (key == KEYCODE_UP ? -1 : 1), 0,
                              int(lines.size()) - 1);
      int col = mCursorPos - lines[row].first;
      mCursorPos = std::min(lines[target].first + col, lines[target].second);
      while (mCursorPos > lines[target].first &&
             (static_cast<unsigned char>(mString[mCursorPos]) & 0xc0) == 0x80)
        --mCursorPos;
      mHilitePos = -1;
      MarkDirty();
      return;
    }
    EditWidget::KeyDown(key);
  }
  int GetCharAt(int x, int y) override {
    auto lines = Lines();
    int row = std::clamp(topLine + std::max(0, y - 5) / LineHeight(), 0,
                         int(lines.size()) - 1);
    int start = lines[row].first, end = lines[row].second, pos = start;
    while (pos < end) {
      int next = static_cast<int>(UTF8NextBoundary(mString, pos));
      if (mFont->StringWidth(
              std::string_view(mString).substr(start, next - start)) > x - 4)
        break;
      pos = next;
    }
    return pos;
  }
  void Draw(Graphics *g) override {
    g->SetColor(mColors.mBkg);
    g->FillRect(0, 0, mWidth, mHeight);
    g->SetColor(mHasFocus ? Color(90, 140, 200) : mColors.mOutline);
    g->DrawRect(0, 0, mWidth - 1, mHeight - 1);
    if (!mFont)
      return;
    auto lines = Lines();
    int current = 0;
    for (int i = 0; i < int(lines.size()); ++i)
      if (mCursorPos >= lines[i].first)
        current = i;
    int visible = std::max(1, (mHeight - 8) / LineHeight());
    if (current < topLine)
      topLine = current;
    if (current >= topLine + visible)
      topLine = current - visible + 1;
    Graphics clip(*g);
    clip.ClipRect(4, 4, mWidth - 8, mHeight - 8);
    clip.SetFont(mFont.get());
    for (int i = topLine; i < std::min(int(lines.size()), topLine + visible);
         ++i) {
      auto [start, end] = lines[i];
      int y = 5 + (i - topLine) * LineHeight() + mFont->GetAscent();
      if (mHilitePos >= 0) {
        int a = std::max(start, std::min(mCursorPos, mHilitePos)),
            b = std::min(end, std::max(mCursorPos, mHilitePos));
        if (a < b) {
          int left = mFont->StringWidth(
              std::string_view(mString).substr(start, a - start));
          clip.SetColor(mColors.mHilite);
          clip.FillRect(
              4 + left, y - mFont->GetAscent(),
              mFont->StringWidth(std::string_view(mString).substr(a, b - a)),
              LineHeight());
        }
      }
      clip.SetColor(mColors.mText);
      clip.DrawString(std::string_view(mString).substr(start, end - start), 4,
                      y);
      if (mHasFocus && mShowingCursor && i == current) {
        int cx = 4 + mFont->StringWidth(std::string_view(mString).substr(
                         start, mCursorPos - start));
        clip.DrawLine(cx, y - mFont->GetAscent(), cx, y + 3);
      }
    }
  }
};
} // namespace Trainer
