// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef PVZ_TRAINER
#include "TrainerPanel.h"
#include "Resources.h"
#include "SexyAppFramework/SexyAppBase.h"
#include "TrainerFiles.h"
#include "TrainerFont.h"
#include "TrainerFormats.h"
#include "TrainerNames.h"
#include "graphics/Font.h"
#include "widget/ButtonWidget.h"
#include "widget/EditWidget.h"
#include "widget/WidgetManager.h"
#include <SDL.h>
#include <algorithm>
#include <charconv>
#include <format>

namespace Trainer {
using namespace Sexy;
namespace {
constexpr int Tab = 10, Row = 100, ExecuteId = 200, Language = 201, Close = 202,
              AllOff = 203, Prev = 204, Next = 205, Copy = 206, Paste = 207,
              Pause = 208, ValueDown = 209, ValueUp = 210, Place = 211,
              SourcePath = 212, DestPath = 213, Share = 214, Types = 215;
unsigned Parameters(Action a) {
  switch (a) {
  case Action::Plant:
    return 15;
  case Action::Zombie:
    return 7;
  case Action::Grave:
  case Action::Ladder:
  case Action::Rake:
    return 6;
  case Action::Slot:
    return 13;
  case Action::MixMode:
    return 9;
  case Action::Win:
    return 8;
  case Action::SpawnGenerate:
    return 15;
  case Action::Sun:
  case Action::Money:
  case Action::TreeHeight:
  case Action::Endless:
  case Action::Mowers:
  case Action::ClearItems:
  case Action::Scene:
  case Action::LoadLineup:
  case Action::Music:
  case Action::Debug:
  case Action::Speed:
    return 1;
  default:
    return 0;
  }
}
bool UsesText(Action a) {
  switch (a) {
  case Action::ImportLineup:
  case Action::ExportLineup:
  case Action::SaveLineup:
  case Action::LoadLineup:
  case Action::SpawnGenerate:
  case Action::SpawnRead:
  case Action::SpawnApply:
  case Action::SpawnSave:
  case Action::SpawnLoad:
  case Action::Screenshot:
  case Action::Pack:
  case Action::Unpack:
  case Action::About:
    return true;
  default:
    return false;
  }
}
bool CanApply(Action a, const Snapshot &state) {
  if (state.replay)
    return false;
  switch (a) {
  case Action::Money:
  case Action::TreeHeight:
  case Action::Unlock:
  case Action::MixMode:
    return state.profile;
  case Action::Music:
  case Action::SaveFolder:
  case Action::Pack:
  case Action::Unpack:
  case Action::Documentation:
  case Action::About:
  case Action::Screenshot:
  case Action::Reset:
  case Action::Speed:
    return true;
  default:
    return state.board;
  }
}
class ToolButton : public ButtonWidget {
public:
  ToolButton(int id, ButtonListener *listener) : ButtonWidget(id, listener) {}
  void Draw(Graphics *g) override {
    g->SetColor(mDisabled        ? Color(45, 55, 50)
                : IsButtonDown() ? Color(39, 84, 62)
                : mIsOver        ? Color(61, 89, 72)
                                 : mColors.mBkg);
    g->FillRect(0, 0, mWidth, mHeight);
    g->SetColor(mIsOver ? Color(142, 193, 150) : Color(79, 103, 86));
    g->DrawRect(0, 0, mWidth - 1, mHeight - 1);
    if (mFont) {
      g->SetFont(mFont.get());
      g->SetColor(mDisabled ? Color(125, 133, 128) : Color(231, 239, 224));
      std::string label = mLabel;
      while (!label.empty() && mFont->StringWidth(label) > mWidth - 18) {
        size_t n = label.size() - 1;
        while (n > 0 && (static_cast<unsigned char>(label[n]) & 0xc0) == 0x80)
          --n;
        label.resize(n);
      }
      g->DrawString(label,
                    mLabelJustify == BUTTON_LABEL_LEFT
                        ? 10
                        : (mWidth - mFont->StringWidth(label)) / 2,
                    (mHeight - mFont->GetHeight()) / 2 + mFont->GetAscent());
    }
  }
};
class TextArea : public EditWidget {
  int topLine = 0;
  std::vector<std::pair<int, int>> Lines() const {
    std::vector<std::pair<int, int>> lines;
    int start = 0;
    for (int i = 0; i < int(mString.size()); ++i)
      if (mString[i] == '\n') {
        lines.emplace_back(start, i);
        start = i + 1;
      }
    lines.emplace_back(start, int(mString.size()));
    return lines;
  }

public:
  TextArea(int id, EditListener *l) : EditWidget(id, l) {}
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
    int row =
        std::clamp(topLine + std::max(0, y - 5) / 23, 0, int(lines.size()) - 1);
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
    g->SetColor(mHasFocus ? Color(149, 193, 145) : mColors.mOutline);
    g->DrawRect(0, 0, mWidth - 1, mHeight - 1);
    if (!mFont)
      return;
    auto lines = Lines();
    int current = 0;
    for (int i = 0; i < int(lines.size()); ++i)
      if (mCursorPos >= lines[i].first)
        current = i;
    int visible = std::max(1, (mHeight - 8) / 23);
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
      int y = 5 + (i - topLine) * 23 + mFont->GetAscent();
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
              23);
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
} // namespace
const char *Panel::Tr(const char *en, const char *zh) const {
  return service.chinese ? zh : en;
}
Panel::Panel(Service &s, std::function<void()> c,
             std::function<void(Command)> p)
    : service(s), close(std::move(c)), place(std::move(p)) {
  font = CreateFont(17);
  titleFont = CreateFont(25);
  mWantsFocus = true;
  if (!font && FONT_BRIANNETOD12)
    font.reset(FONT_BRIANNETOD12->Duplicate());
  if (!titleFont && font)
    titleFont.reset(font->Duplicate());
  if (!HasChineseFont()) {
    service.chinese = false;
    service.messageEn = "Chinese font unavailable; place trainer-font.ttf in "
                        "the resource folder.";
  }
  auto add = [&](int id) {
    auto b = std::make_unique<ToolButton>(id, this);
    b->SetFont(font.get());
    b->SetBkgColor(Color(43, 63, 51));
    AddWidget(b.get());
    buttons.push_back(std::move(b));
  };
  for (int i = 0; i < 5; ++i)
    add(Tab + i);
  for (int i = 0; i < 12; ++i)
    add(Row + i);
  for (int id = ExecuteId; id <= Types; ++id)
    add(id);
  for (int i = 0; i < 5; ++i) {
    if (i == 4)
      edits[i] = std::make_unique<TextArea>(i, this);
    else
      edits[i] = std::make_unique<EditWidget>(i, this);
    edits[i]->SetFont(font.get());
    edits[i]->SetColors({Color(22, 32, 27), Color(83, 110, 92),
                         Color(236, 240, 228), Color(69, 117, 88),
                         Color::White});
    edits[i]->mMaxChars = i == 4 ? 250000 : 12;
    edits[i]->SetText(i == 1 || i == 2 ? "1" : i == 4 ? "" : "0");
    AddWidget(edits[i].get());
  }
  Resize(0, 0, 960, 700);
  Refresh();
}
Panel::~Panel() {
  for (auto &b : buttons)
    RemoveWidget(b.get());
  for (auto &e : edits)
    RemoveWidget(e.get());
}
void Panel::Resize(int x, int y, int w, int h) {
  Widget::Resize(x, y, w, h);
  int left = std::clamp(w * 39 / 100, 260, 380), right = left + 30,
      rw = w - right - 20;
  listRows = std::clamp((h - 250) / 42, 4, 12);
  for (auto &b : buttons) {
    int id = b->mId;
    if (id >= Tab && id < Tab + 5)
      b->Resize(20 + (id - Tab) * (w - 40) / 5, 89, (w - 40) / 5 - 5, 39);
    else if (id >= Row && id < Row + 12) {
      int i = id - Row;
      b->Resize(20, 144 + i * 42, left - 5, 37);
      b->mLabelJustify = ButtonWidget::BUTTON_LABEL_LEFT;
      b->SetVisible(i < listRows);
    } else if (id == Language)
      b->Resize(w - 225, 18, 120, 34);
    else if (id == Close)
      b->Resize(w - 95, 18, 75, 34);
    else if (id == AllOff)
      b->Resize(w - 150, h - 49, 130, 32);
    else if (id == Prev)
      b->Resize(20, h - 110, 65, 31);
    else if (id == Next)
      b->Resize(left - 50, h - 110, 65, 31);
    else if (id == ExecuteId)
      b->Resize(right, h - 120, rw, 36);
    else if (id == Copy)
      b->Resize(right, h - 162, rw / 3 - 4, 32);
    else if (id == Paste)
      b->Resize(right + rw / 3 + 4, h - 162, rw / 3 - 8, 32);
    else if (id == Pause)
      b->Resize(w - 330, h - 49, 165, 32);
    else if (id == ValueDown)
      b->Resize(right, 245, 28, 32);
    else if (id == ValueUp)
      b->Resize(right + rw / 2 - 36, 245, 28, 32);
    else if (id == Place)
      b->Resize(right, h - 120, rw / 2 - 4, 36);
    else if (id == SourcePath)
      b->Resize(right + rw - 200, 337, 95, 21);
    else if (id == DestPath)
      b->Resize(right + rw - 100, 337, 100, 21);
    else if (id == Share)
      b->Resize(right + 2 * rw / 3 + 4, h - 162, rw / 3 - 4, 32);
    else if (id == Types)
      b->Resize(right + rw / 2 - 94, 216, 88, 24);
  }
  for (int i = 0; i < 4; ++i)
    edits[i]->Resize(right + (i % 2) * (rw / 2 + 4), i < 2 ? 245 : 300,
                     rw / 2 - 8, 32);
  edits[0]->Resize(right + 32, 245, rw / 2 - 72, 32);
  edits[4]->Resize(right, 358, rw, std::max(40, h - 527));
  needsRefresh = true;
}
void Panel::Refresh() {
  indices.clear();
  const auto &features = Features();
  if (choosingTypes && selected >= 0) {
    auto a = features[selected].action;
    int count = a == Action::MixMode ? int(ModeNames.size())
                : a == Action::Zombie || a == Action::SpawnGenerate
                    ? int(ZombieNames.size())
                    : int(PlantNames.size());
    for (int i = 0; i < count; ++i)
      indices.push_back(i);
  } else
    for (int i = 0; i < static_cast<int>(features.size()); ++i)
      if (features[i].page == page)
        indices.push_back(i);
  offset = std::clamp(offset, 0,
                      std::max(0, static_cast<int>(indices.size()) - listRows));
  if (selected < 0 || selected >= static_cast<int>(features.size()) ||
      features[selected].page != page)
    selected = indices.empty() ? -1 : indices.front();
  static const char *en[] = {"Resources", "Battlefield", "Lineup", "Spawning",
                             "Utilities"};
  static const char *zh[] = {"资源", "战场", "阵型", "出怪", "杂项"};
  for (auto &b : buttons) {
    int id = b->mId;
    b->SetBkgColor(Color(43, 63, 51));
    if (id >= Tab && id < Tab + 5) {
      b->mLabel = service.chinese ? zh[id - Tab] : en[id - Tab];
      if (id - Tab == page)
        b->SetBkgColor(Color(65, 103, 70));
    } else if (id >= Row && id < Row + 12) {
      int n = offset + id - Row;
      bool visible =
          id - Row < listRows && n < static_cast<int>(indices.size());
      b->SetVisible(visible);
      if (!visible)
        continue;
      if (choosingTypes) {
        int type = indices[n];
        auto a = features[selected].action;
        auto name = a == Action::MixMode ? ModeNames[type]
                    : a == Action::Zombie || a == Action::SpawnGenerate
                        ? ZombieNames[type]
                        : PlantNames[type];
        b->mLabel = std::to_string(type) + "  " + Tr(name.first, name.second);
      } else {
        auto &f = features[indices[n]];
        b->mLabel =
            std::string(f.flag >= 0 ? service.Get(static_cast<Flag>(f.flag))
                                          ? "[x] "
                                          : "[ ] "
                                    : "") +
            Tr(f.en, f.zh);
        if (indices[n] == selected)
          b->SetBkgColor(Color(65, 103, 70));
      }
    } else
      switch (id) {
      case ExecuteId: {
        bool canPlace = selected >= 0 && features[selected].flag < 0 &&
                        (features[selected].action == Action::Plant ||
                         features[selected].action == Action::Zombie ||
                         features[selected].action == Action::Grave ||
                         features[selected].action == Action::Ladder ||
                         features[selected].action == Action::Rake);
        int left = std::clamp(mWidth * 39 / 100, 260, 380), x = left + 30,
            rw = mWidth - x - 20;
        b->Resize(canPlace ? x + rw / 2 + 4 : x, mHeight - 120,
                  canPlace ? rw / 2 - 4 : rw, 36);
        bool flag = selected >= 0 && features[selected].flag >= 0;
        b->mLabel =
            flag ? Tr("Toggle effect", "切换效果") : Tr("Apply", "应用");
        b->SetDisabled(
            selected < 0 || service.state.replay ||
            (!flag && !CanApply(features[selected].action, service.state)));
        break;
      }
      case Language:
        b->mLabel = service.chinese ? "English" : "中文";
        break;
      case Close:
        b->mLabel = Tr("Close", "关闭");
        break;
      case AllOff:
        b->mLabel = Tr("All effects off", "关闭全部效果");
        break;
      case Prev:
        b->mLabel = "<";
        b->SetDisabled(offset == 0);
        break;
      case Next:
        b->mLabel = ">";
        b->SetDisabled(offset + listRows >= static_cast<int>(indices.size()));
        break;
      case Copy:
        b->mLabel = Tr("Copy text", "复制内容");
        b->SetVisible(selected >= 0 && features[selected].flag < 0 &&
                      UsesText(features[selected].action));
        break;
      case Paste:
        b->mLabel = Tr("Paste text", "粘贴内容");
        b->SetVisible(selected >= 0 && features[selected].flag < 0 &&
                      UsesText(features[selected].action));
        break;
      case Pause:
        b->mLabel = service.paused ? Tr("Resume game", "继续游戏")
                                   : Tr("Pause game", "暂停游戏");
        b->SetDisabled(!service.state.board || service.state.replay);
        break;
      case ValueDown:
        b->mLabel = "-";
        break;
      case ValueUp:
        b->mLabel = "+";
        break;
      case Types: {
        auto a = selected >= 0 ? features[selected].action : Action::Reset;
        b->SetVisible(selected >= 0 && features[selected].flag < 0 &&
                      (a == Action::Plant || a == Action::Zombie ||
                       a == Action::Slot || a == Action::SpawnGenerate ||
                       a == Action::MixMode));
        b->mLabel =
            choosingTypes ? Tr("Done", "完成选择") : Tr("Types", "类型列表");
        break;
      }
      case SourcePath:
      case DestPath:
      case Share: {
        Action a = selected >= 0 ? features[selected].action : Action::Reset;
        bool file = a == Action::Pack || a == Action::Unpack ||
                    a == Action::SaveLineup || a == Action::LoadLineup ||
                    a == Action::SpawnSave || a == Action::SpawnLoad ||
                    a == Action::Screenshot;
        b->SetVisible(
            file && (a != Action::Screenshot || id == Share) &&
            (id != DestPath || a == Action::Pack || a == Action::Unpack));
        if (id != Share)
          b->SetDisabled(!CanChooseFiles());
        b->mLabel = id == SourcePath ? Tr("Choose", "选择")
                    : id == DestPath ? Tr("Output", "输出")
                                     : Tr("Export file", "导出文件");
        break;
      }
      case Place: {
        bool canPlace = selected >= 0 && features[selected].flag < 0 &&
                        (features[selected].action == Action::Plant ||
                         features[selected].action == Action::Zombie ||
                         features[selected].action == Action::Grave ||
                         features[selected].action == Action::Ladder ||
                         features[selected].action == Action::Rake);
        b->SetVisible(canPlace);
        b->SetDisabled(!service.state.board || service.state.replay);
        b->mLabel = Tr("Place on lawn", "点击战场放置");
        break;
      }
      }
  }
  unsigned params = selected >= 0 && features[selected].flag < 0
                        ? Parameters(features[selected].action)
                        : 0;
  for (int i = 0; i < 4; ++i)
    edits[i]->SetVisible((params & (1 << i)) != 0);
  edits[4]->SetVisible(selected >= 0 && features[selected].flag < 0 &&
                       UsesText(features[selected].action));
  for (auto &b : buttons)
    if (b->mId == ValueDown || b->mId == ValueUp)
      b->SetVisible((params & 1) != 0);
  lastChinese = service.chinese;
  needsRefresh = false;
  MarkDirty();
}
void Panel::Select(int i) {
  selected = i;
  const auto &f = Features()[i];
  if (f.flag >= 0)
    service.Set(static_cast<Flag>(f.flag),
                !service.Get(static_cast<Flag>(f.flag)));
  else {
    int v = 0;
    switch (f.action) {
    case Action::Sun:
      v = 9990;
      break;
    case Action::Money:
      v = 999990;
      break;
    case Action::Speed:
      v = 100;
      break;
    case Action::TreeHeight:
      v = 1000;
      break;
    default:
      break;
    }
    edits[0]->SetText(std::to_string(v));
    edits[1]->SetText("1");
    edits[2]->SetText("1");
    edits[3]->SetText("0");
    if (f.action == Action::SpawnGenerate) {
      edits[1]->SetText("0");
      edits[2]->SetText("1000");
      edits[3]->SetText("50");
    }
    if (f.action == Action::ClearItems)
      edits[0]->SetText("-1");
    if (f.action == Action::MixMode)
      edits[3]->SetText("1");
  }
  needsRefresh = true;
}
Command Panel::ReadCommand(bool &valid) {
  Command c;
  valid = false;
  if (selected < 0)
    return c;
  c.action = Features()[selected].action;
  c.generation = service.state.generation;
  int *vals[] = {&c.value, &c.row, &c.col, &c.extra};
  for (int i = 0; i < 4; ++i) {
    auto &str = edits[i]->mString;
    auto r = std::from_chars(str.data(), str.data() + str.size(), *vals[i]);
    if (r.ec != std::errc{} || r.ptr != str.data() + str.size()) {
      service.messageEn = "Enter whole numbers in the four numeric fields.";
      service.messageZh = "请在四个数值框中输入完整整数。";
      return c;
    }
  }
  if (c.action != Action::SpawnGenerate) {
    --c.row;
    --c.col;
  }
  c.text = edits[4]->mString;
  valid = true;
  return c;
}
void Panel::Execute() {
  if (selected < 0)
    return;
  const auto &f = Features()[selected];
  if (f.flag >= 0) {
    service.Set(static_cast<Flag>(f.flag),
                !service.Get(static_cast<Flag>(f.flag)));
    needsRefresh = true;
    return;
  }
  bool valid;
  auto c = ReadCommand(valid);
  if (valid)
    service.Submit(std::move(c));
}
void Panel::ButtonDepress(int id) {
  if (id >= Tab && id < Tab + 5) {
    page = id - Tab;
    offset = 0;
    selected = -1;
    choosingTypes = false;
    needsRefresh = true;
  } else if (id >= Row && id < Row + 12) {
    int n = offset + id - Row;
    if (n < static_cast<int>(indices.size())) {
      if (!choosingTypes)
        Select(indices[n]);
      else if (Features()[selected].action == Action::SpawnGenerate) {
        std::vector<int> ids;
        try {
          ids = Formats::ParseIntegers(edits[4]->mString, 0, 32, 33);
        } catch (...) {
        }
        int type = indices[n];
        auto pos = std::find(ids.begin(), ids.end(), type);
        if (pos == ids.end())
          ids.push_back(type);
        else
          ids.erase(pos);
        std::string text;
        for (int t : ids) {
          if (!text.empty())
            text += ' ';
          text += std::to_string(t);
        }
        edits[4]->SetText(text);
      } else {
        edits[0]->SetText(std::to_string(indices[n]));
        choosingTypes = false;
        offset = featureOffset;
      }
      needsRefresh = true;
    }
  } else
    switch (id) {
    case Language:
      if (!service.chinese && !HasChineseFont()) {
        service.messageEn =
            "Install a Chinese font or supply trainer-font.ttf in resources.";
        break;
      }
      service.chinese = !service.chinese;
      service.SaveSettings();
      needsRefresh = true;
      break;
    case Types:
      if (choosingTypes) {
        choosingTypes = false;
        offset = featureOffset;
      } else {
        choosingTypes = true;
        featureOffset = offset;
        offset = 0;
      }
      needsRefresh = true;
      break;
    case Close:
      close();
      break;
    case AllOff:
      service.AllOff();
      needsRefresh = true;
      break;
    case Prev:
      offset = std::max(0, offset - listRows);
      needsRefresh = true;
      break;
    case Next:
      offset += listRows;
      needsRefresh = true;
      break;
    case ExecuteId:
      Execute();
      break;
    case Place: {
      bool valid;
      auto c = ReadCommand(valid);
      if (valid && place)
        place(std::move(c));
      break;
    }
    case SourcePath:
    case DestPath: {
      if (selected < 0)
        break;
      Action action = Features()[selected].action;
      bool output = id == DestPath;
      int selection = selected;
      auto done = [this, action, output, selection](std::string path) {
        if (path.empty() || selected != selection)
          return;
        if (action == Action::Pack || action == Action::Unpack) {
          auto current = edits[4]->mString;
          auto split = current.find('\n');
          std::string source =
              split == std::string::npos ? current : current.substr(0, split);
          std::string dest =
              split == std::string::npos ? "" : current.substr(split + 1);
          if (output) {
            if (action == Action::Unpack)
              path += "/pvz-unpacked-" + std::to_string(SDL_GetTicks());
            dest = path;
          } else
            source = path;
          edits[4]->SetText(source + "\n" + dest);
        } else
          edits[4]->SetText(path);
      };
#if defined(__ANDROID__) || defined(__IPHONEOS__)
      if (action == Action::Unpack && output) {
        ChooseFile(true, [done = std::move(done)](std::string path) mutable {
          if (!path.empty()) {
            auto local = Sexy::PathFromU8(path);
            local.replace_extension();
            path = Sexy::PathToU8(local);
          }
          done(std::move(path));
        });
        break;
      }
#endif
      if ((action == Action::Pack && !output) ||
          (action == Action::Unpack && output))
        ChooseDirectory(std::move(done));
      else
        ChooseFile(output || action == Action::SaveLineup ||
                       action == Action::SpawnSave,
                   std::move(done));
      break;
    }
    case Share: {
      auto path = edits[4]->mString;
      auto split = path.find('\n');
      if (split != std::string::npos)
        path = path.substr(split + 1);
      if (!ShareFile(path)) {
        service.messageEn = "Select an existing output file first.";
        service.messageZh = "请先选择已生成的输出文件。";
      }
      break;
    }
    case Pause:
      if (!service.state.replay) {
        service.paused = !service.paused;
        needsRefresh = true;
      }
      break;
    case Copy:
      if (mWidgetManager)
        mWidgetManager->mApp->CopyToClipboard(edits[4]->mString);
      break;
    case Paste:
      if (mWidgetManager)
        edits[4]->SetText(mWidgetManager->mApp->GetClipboard());
      break;
    case ValueDown:
    case ValueUp: {
      int v = 0;
      auto &s = edits[0]->mString;
      auto r = std::from_chars(s.data(), s.data() + s.size(), v);
      if (r.ec == std::errc{} && v > -100000000 && v < 100000000)
        edits[0]->SetText(std::to_string(v + (id == ValueUp ? 1 : -1)));
      break;
    }
    }
}
void Panel::EditWidgetText(int, const std::string &) { Execute(); }
void Panel::MouseWheel(int delta) {
  offset -= delta * 2;
  needsRefresh = true;
}
void Panel::Update() {
  Widget::Update();
  if (previousState.generation != service.state.generation ||
      previousState.profile != service.state.profile ||
      previousState.replay != service.state.replay ||
      previousPaused != service.paused)
    needsRefresh = true;
  previousState = service.state;
  previousPaused = service.paused;
  if (lastChinese != service.chinese || needsRefresh)
    Refresh();
  if (lastText != service.text) {
    lastText = service.text;
    edits[4]->SetText(lastText);
  }
  MarkDirty();
}
void Panel::Wrap(Graphics *g, std::string_view str, int x, int y, int width,
                 int maxLines) {
  std::string line;
  int lines = 0;
  for (size_t i = 0; i < str.size();) {
    size_t n = 1;
    unsigned char ch = str[i];
    if (ch >= 0xf0)
      n = 4;
    else if (ch >= 0xe0)
      n = 3;
    else if (ch >= 0xc0)
      n = 2;
    n = std::min(n, str.size() - i);
    std::string next(str.substr(i, n));
    i += n;
    if (next == "\n" || font->StringWidth(line + next) > width) {
      g->DrawString(line, x, y);
      line.clear();
      y += 23;
      if (++lines >= maxLines)
        return;
    }
    if (next != "\n")
      line += next;
  }
  if (!line.empty() && lines < maxLines)
    g->DrawString(line, x, y);
}
void Panel::Draw(Graphics *g) {
  int left = std::clamp(mWidth * 39 / 100, 260, 380), x = left + 30,
      rw = mWidth - x - 20;
  g->SetColor(Color(24, 36, 29));
  g->FillRect(0, 0, mWidth, mHeight);
  g->SetColor(Color(37, 53, 42));
  g->FillRect(0, 0, mWidth, 78);
  g->SetFont(titleFont.get());
  g->SetColor(Color(227, 239, 210));
  g->DrawString(Tr("PvZ Toolkit", "PvZ 工具箱"), 20, 39);
  g->SetFont(font.get());
  g->SetColor(Color(167, 191, 168));
  auto &s = service.state;
  std::string status =
      s.board ? std::vformat(Tr("Sun {}  Coins {}  Wave {}  Speed {:.2f}x",
                                "阳光 {}  金币 {}  波次 {}  速度 {:.2f}x"),
                             std::make_format_args(s.sun, s.coins, s.wave,
                                                   service.speed))
              : Tr("Main menu / no active lawn", "主菜单 / 当前没有战场");
  g->DrawString(status, 20, 66);
  g->SetColor(Color(70, 93, 76));
  g->FillRect(left + 12, 143, 1, mHeight - 255);
  if (selected >= 0) {
    auto &f = Features()[selected];
    g->SetColor(Color(238, 233, 198));
    g->DrawString(Tr(f.en, f.zh), x, 165);
    g->SetColor(Color(177, 194, 177));
    Wrap(g, Tr(f.hintEn, f.hintZh), x, 194, rw, 2);
  }
  g->SetColor(Color(167, 191, 168));
  Action action = selected >= 0 ? Features()[selected].action : Action::Reset;
  unsigned params =
      selected >= 0 && Features()[selected].flag < 0 ? Parameters(action) : 0;
  bool spawn = action == Action::SpawnGenerate;
  if (params & 1)
    g->DrawString(Tr("Value / type ID", "数值 / 类型编号"), x, 237);
  if (params & 2)
    g->DrawString(spawn ? Tr("Red-eye limit 0/1", "红眼限制 0/1")
                        : Tr("Row (1-based)", "行（从 1 开始）"),
                  x + rw / 2 + 4, 237);
  if (params & 4)
    g->DrawString(spawn ? Tr("Red-eye weight", "红眼权重")
                        : Tr("Column (1-based)", "列（从 1 开始）"),
                  x, 292);
  if (params & 8)
    g->DrawString(spawn ? Tr("Enemies per wave", "每波僵尸数量")
                        : Tr("Extra / imitater", "附加值 / 模仿者"),
                  x + rw / 2 + 4, 292);
  if (selected >= 0 && Features()[selected].flag < 0 && UsesText(action))
    g->DrawString(Tr("Text / files", "文本 / 文件"), x, 350);
  g->SetColor(Color(177, 194, 177));
  std::string msg = service.chinese ? service.messageZh : service.messageEn;
  if (s.replay)
    msg = Tr("Tools disabled during recording / replay.",
             "录制或回放期间，修改功能已禁用。");
  Wrap(g, msg, 20, mHeight - 66, mWidth - 40, 1);
  int enabled = 0;
  for (int i = 0; i < static_cast<int>(Flag::Count); ++i)
    enabled += service.Get(static_cast<Flag>(i));
  g->DrawString(std::vformat(Tr("{} active effects  |  F10: tools",
                                "已启用 {} 项效果  |  F10：工具箱"),
                             std::make_format_args(enabled)),
                20, mHeight - 26);
  g->DrawString(
      std::format("{}-{} / {}", indices.empty() ? 0 : offset + 1,
                  std::min(offset + listRows, static_cast<int>(indices.size())),
                  indices.size()),
      95, mHeight - 89);
}
} // namespace Trainer
#endif
