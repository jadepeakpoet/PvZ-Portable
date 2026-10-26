// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef PVZ_TRAINER
#include "TrainerPanel.h"
#include "Resources.h"
#include "SexyAppFramework/SexyAppBase.h"
#include "TrainerFiles.h"
#include "TrainerFont.h"
#include "TrainerFormats.h"
#include "TrainerNames.h"
#include "TrainerTextArea.h"
#include "widget/ButtonWidget.h"
#include "widget/EditWidget.h"
#include "widget/WidgetManager.h"
#include <SDL.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <fstream>
#include <sstream>
namespace Trainer {
using namespace Sexy;
namespace {
constexpr int MenuFirst = 5000, MenuUp = 4900, MenuDown = 4901;
constexpr int spawnTypes[] = {2,  3,  4,  5,  6,  7,  8,  11, 12, 14,
                              15, 16, 17, 18, 19, 20, 21, 22, 23, 32};
constexpr int speedValues[] = {1000, 500, 200, 100, 50, 20, 10};
constexpr int lilyFrom[] = {1, 1, 1, 1, 1, 1, 1, 1},
              lilyTo[] = {2, 3, 4, 5, 6, 7, 8, 9};
constexpr int potFrom[] = {1, 4, 4, 4, 4, 4, 4},
              potTo[] = {3, 4, 5, 6, 7, 8, 9};
class ClassicButton final : public ButtonWidget {
public:
  Classic::Kind kind = Classic::Kind::Button;
  bool checked = false, activeTab = false;
  double scale = 1;
  ClassicButton(int id, ButtonListener *l) : ButtonWidget(id, l) {}
  void Draw(Graphics *g) override {
    const auto bg = Color(243, 243, 243), border = Color(171, 171, 171);
    const int unit = std::max(1, int(std::round(scale)));
    int tx = 4 * scale;
    if (kind == Classic::Kind::Check) {
      g->SetColor(bg);
      g->FillRect(0, 0, mWidth, mHeight);
      int size = std::max(8, int(12 * scale)), y = (mHeight - size) / 2;
      g->SetColor(mDisabled ? bg : Color::White);
      g->FillRect(2 * scale, y, size, size);
      g->SetColor(Color(115, 115, 115));
      g->DrawRect(2 * scale, y, size - 1, size - 1);
      if (checked) {
        g->SetColor(mDisabled ? Color(150, 150, 150) : Color::Black);
        for (int n = 0; n < unit; ++n) {
          g->DrawLine(4 * scale, y + size / 2 + n, 7 * scale,
                      y + size - 3 * scale + n);
          g->DrawLine(7 * scale, y + size - 3 * scale + n, 12 * scale,
                      y + 3 * scale + n);
        }
      }
      tx = 18 * scale;
    } else {
      g->SetColor(kind == Classic::Kind::Choice ? Color::White
                  : activeTab                   ? Color::White
                  : IsButtonDown()              ? Color(222, 222, 222)
                  : mIsOver                     ? Color(232, 239, 245)
                                                : bg);
      g->FillRect(0, 0, mWidth, mHeight);
      g->SetColor(activeTab ? Color(120, 120, 120) : border);
      g->DrawRect(0, 0, mWidth - 1, mHeight - 1);
      if (kind == Classic::Kind::Choice) {
        int aw = std::min(mWidth / 3, int(21 * scale));
        g->SetColor(Color(238, 238, 238));
        g->FillRect(mWidth - aw, unit, aw - unit, mHeight - 2 * unit);
        g->SetColor(border);
        g->DrawLine(mWidth - aw, 0, mWidth - aw, mHeight - 1);
        g->SetColor(mDisabled ? Color(172, 172, 172) : Color::Black);
        int cx = mWidth - aw / 2, cy = mHeight / 2;
        for (int i = 0; i < 4 * scale; ++i)
          g->DrawLine(cx - int(4 * scale) + i, cy - int(2 * scale) + i,
                      cx + int(4 * scale) - i, cy - int(2 * scale) + i);
      }
    }
    if (!mFont)
      return;
    g->SetFont(mFont.get());
    g->SetColor(mDisabled ? Color(165, 165, 165) : Color::Black);
    std::string label = mLabel;
    int limit = mWidth - tx -
                (kind == Classic::Kind::Choice ? int(25 * scale) : 3 * scale);
    while (!label.empty() && mFont->StringWidth(label) > limit) {
      size_t pos = label.size() - 1;
      while (pos > 0 && (static_cast<unsigned char>(label[pos]) & 0xc0) == 0x80)
        --pos;
      label.resize(pos);
    }
    if (kind == Classic::Kind::Button && mLabelJustify != BUTTON_LABEL_LEFT)
      tx = (mWidth - mFont->StringWidth(label)) / 2;
    g->DrawString(label, tx,
                  (mHeight - mFont->GetHeight()) / 2 + mFont->GetAscent());
  }
};
class DetailsOverlay final : public Widget {
  Panel &panel;
  int lastX = 0, lastY = 0, holdTicks = 0;
  bool held = false, dragged = false;

public:
  explicit DetailsOverlay(Panel &p) : panel(p) { mWantsFocus = true; }
  void Draw(Graphics *g) override { panel.DrawDetails(g, mWidth, mHeight); }
  void MouseDown(int x, int y, int button, int) override {
    lastX = x;
    lastY = y;
    held = button == 1;
    dragged = false;
    holdTicks = 0;
    if (!held)
      panel.DetailsClick(x, y, button);
  }
  void MouseDrag(int x, int y) override {
    int dx = (lastX - x) / 36, dy = (lastY - y) / 18;
    if (held && (dx || dy)) {
      dragged = true;
      panel.ScrollDetails(dx, dy);
      lastX -= dx * 36;
      lastY -= dy * 18;
    }
  }
  void MouseUp(int x, int y, int button, int) override {
    if (held && !dragged && button == 1)
      panel.DetailsClick(x, y, 1);
    held = false;
  }
  void Update() override {
    Widget::Update();
    if (held && !dragged && ++holdTicks == 40) {
      panel.DetailsClick(lastX, lastY, -1);
      dragged = true;
    }
  }
  void MouseWheel(int delta) override { panel.ScrollDetails(0, -delta); }
  void KeyDown(KeyCode key) override {
    if (key == KEYCODE_ESCAPE)
      panel.detailsOpen = false;
  }
};
std::string ShortZombie(int id, bool zh) {
  static constexpr const char *names[] = {
      "路障", "撑杆", "铁桶", "读报", "铁门", "橄榄", "舞王",
      "潜水", "冰车", "海豚", "小丑", "气球", "矿工", "跳跳",
      "雪人", "蹦极", "扶梯", "投篮", "白眼", "红眼"};
  for (int i = 0; i < 20; ++i)
    if (spawnTypes[i] == id)
      return zh ? names[i] : ZombieNames[id].first;
  return "";
}
} // namespace
const char *Panel::Tr(const char *en, const char *zh) const {
  return service.chinese ? zh : en;
}
int Panel::PreferredWidth() const { return Classic::Width(service.chinese); }
int Panel::PreferredHeight() const { return Classic::Height(service.chinese); }
Panel::Panel(Service &s, std::function<void()> c,
             std::function<void(Command)> p)
    : service(s), close(std::move(c)), place(std::move(p)) {
  using namespace Classic;
  values = {{Mode, 13},     {Adventure, 39},    {Row, 1},         {Col, 1},
            {ClearType, 3}, {Scene, 2},         {Music, 4},       {Speed, 3},
            {SpawnMode, 1}, {GigaWeight, 1000}, {BrightestCob, 1}};
  text = {{SunInput, "8000"},
          {MoneyInput, "80000"},
          {TreeInput, "1000"},
          {Rounds, "1010"}};
  mWantsFocus = true;
  Widget::Resize(0, 0, PreferredWidth(), PreferredHeight());
  Build();
}
Panel::~Panel() {
  if (detailsOverlay)
    RemoveWidget(detailsOverlay.get());
  CloseMenu();
  for (auto &[id, b] : buttons)
    RemoveWidget(b.get());
  for (auto &[id, e] : edits)
    RemoveWidget(e.get());
}
int Panel::Value(int id) const {
  auto i = values.find(id);
  return i == values.end() ? 0 : i->second;
}
void Panel::SetValue(int id, int value) {
  values[id] = value;
  if (edits.contains(id)) {
    edits.at(id)->SetText(std::to_string(value));
    text[id] = std::to_string(value);
  }
  if (id == Classic::Language) {
    service.chinese = value == 0;
    service.SaveSettings();
    rebuild = true;
  }
  Refresh();
}
int Panel::Number(int id) const {
  auto s = Text(id);
  int value = 0;
  auto r = std::from_chars(s.data(), s.data() + s.size(), value);
  if (r.ec != std::errc{} || r.ptr != s.data() + s.size())
    throw std::runtime_error(Tr("Enter a whole number.", "请输入完整整数。"));
  return value;
}
std::string Panel::Text(int id) const {
  auto i = edits.find(id);
  if (i != edits.end())
    return i->second->mString;
  auto t = text.find(id);
  return t == text.end() ? "" : t->second;
}
void Panel::Build() {
  if (detailsOverlay) {
    RemoveWidget(detailsOverlay.get());
    detailsOverlay.reset();
  }
  for (auto &[id, e] : edits)
    text[id] = e->mString;
  CloseMenu();
  for (auto &[id, b] : buttons)
    RemoveWidget(b.get());
  for (auto &[id, e] : edits)
    RemoveWidget(e.get());
  buttons.clear();
  edits.clear();
  if (!HasChineseFont()) {
    service.chinese = false;
    service.messageEn =
        "Chinese font unavailable: provide trainer-font.ttf in resources.";
  }
  builtChinese = service.chinese;
  values[Classic::Language] = builtChinese ? 0 : 1;
  scale = std::min(double(mWidth) / PreferredWidth(),
                   double(mHeight) / PreferredHeight());
  scale = std::max(.5, scale);
  font = CreateFont(
      std::max(10, int(std::round((builtChinese ? 16 : 14) * scale))));
  if (!font && FONT_BRIANNETOD12)
    font.reset(FONT_BRIANNETOD12->Duplicate());
  layout = Classic::Layout(builtChinese);
  auto button = [&](int id, Classic::Kind kind, const char *label) {
    auto b = std::make_unique<ClassicButton>(id, this);
    b->kind = kind;
    b->scale = scale;
    b->mLabel = label;
    b->SetFont(font.get());
    AddWidget(b.get());
    buttons[id] = std::move(b);
  };
  static const char *zh[] = {"资源", "战场", "阵型", "出怪", "杂项"};
  static const char *en[] = {"Resources", "Battlefield", "Setups", "Spawning",
                             "Others"};
  for (int i = 0; i < 5; ++i)
    button(10 + i, Classic::Kind::Button, builtChinese ? zh[i] : en[i]);
  for (auto &spec : layout) {
    if (spec.kind == Classic::Kind::Label)
      continue;
    if (spec.kind == Classic::Kind::Input || spec.kind == Classic::Kind::Text) {
      std::unique_ptr<EditWidget> e =
          spec.kind == Classic::Kind::Text
              ? std::unique_ptr<EditWidget>(
                    std::make_unique<ClassicTextArea>(spec.id, this))
              : std::make_unique<EditWidget>(spec.id, this);
      e->SetFont(font.get());
      e->SetColors({Color::White, Color(165, 165, 165), Color::Black,
                    Color(0, 120, 215), Color::White});
      e->mMaxChars = spec.kind == Classic::Kind::Text ? 250000 : 2048;
      e->SetText(text[spec.id]);
      AddWidget(e.get());
      edits[spec.id] = std::move(e);
    } else
      button(spec.id, spec.kind, spec.label);
  }
  button(Classic::Close, Classic::Kind::Button, "×");
  detailsOverlay = std::make_unique<DetailsOverlay>(*this);
  AddWidget(detailsOverlay.get());
  Relayout();
  Refresh();
  rebuild = false;
}
void Panel::Relayout() {
  const int W = PreferredWidth(), H = PreferredHeight();
  scale = std::min(double(mWidth) / W, double(mHeight) / H);
  scale = std::max(.5, scale);
  originX = (mWidth - int(W * scale)) / 2;
  originY = (mHeight - int(H * scale)) / 2;
  auto rect = [&](int x, int y, int w, int h) {
    return Rect(originX + std::lround(x * scale),
                originY + std::lround(y * scale),
                std::max(1, int(std::lround(w * scale))),
                std::max(1, int(std::lround(h * scale))));
  };
  for (auto &spec : layout) {
    auto r = rect(spec.x, spec.y, spec.w, spec.h);
    if (buttons.contains(spec.id)) {
      buttons[spec.id]->Resize(r);
      static_cast<ClassicButton *>(buttons[spec.id].get())->scale = scale;
    }
    if (edits.contains(spec.id))
      edits[spec.id]->Resize(r.mX, r.mY, r.mWidth, r.mHeight);
  }
  int m = builtChinese ? 10 : 8, tabW = builtChinese ? 42 : 62,
      th = builtChinese ? 30 : 27;
  for (int i = 0; i < 5; ++i)
    buttons[10 + i]->Resize(rect(m + i * tabW, m, tabW - 1, th));
  buttons[Classic::Close]->Resize(rect(W - 24, m + 2, 16, 22));
  // Native desktop chrome supplies its own close button. The embedded mobile
  // panel uses the original status-tip position for a touch close target.
  buttons[Classic::Close]->SetVisible(embedded);
  if (detailsOverlay)
    detailsOverlay->Resize(0, 0, mWidth, mHeight);
}
void Panel::Resize(int x, int y, int w, int h) {
  Widget::Resize(x, y, w, h);
  rebuild = true;
}
std::vector<std::pair<int, std::string>> Panel::Options(int id) const {
  using namespace Classic;
  std::vector<std::pair<int, std::string>> out;
  auto add = [&](int v, std::string name) {
    out.emplace_back(v, std::move(name));
  };
  if (id == Plant || id == Seed)
    for (int i = 0; i < int(PlantNames.size()); ++i)
      add(i, "[" + std::to_string(i) + "] " +
                 Tr(PlantNames[i].first, PlantNames[i].second));
  else if (id == Zombie)
    for (int i = 0; i < int(ZombieNames.size()); ++i)
      add(i, "[" + std::to_string(i) + "] " +
                 Tr(ZombieNames[i].first, ZombieNames[i].second));
  else if (id == Mode)
    for (int i = 0; i < int(ModeNames.size()); ++i)
      add(i, "[" + std::to_string(i) + "] " +
                 Tr(ModeNames[i].first, ModeNames[i].second));
  else if (id == Row || id == Col) {
    add(0, id == Row ? Tr("All Rows", "所有行") : Tr("All Columns", "所有列"));
    for (int i = 1; i <= (id == Row ? 6 : 9); ++i)
      add(i, std::format("{} {} {}", Tr("", "第"), i,
                         id == Row ? Tr("Row", "行") : Tr("Column", "列")));
  } else if (id == Slot)
    for (int i = 0; i < 10; ++i)
      add(i, std::to_string(i + 1) + Tr(" Seed", " 格卡槽"));
  else if (id == Adventure)
    for (int i = 0; i < 50; ++i)
      add(i, std::format("{} - {}", i / 10 + 1, i % 10 + 1));
  else if (id == Mowers) {
    add(0, Tr("Start", "启动"));
    add(1, Tr("Delete", "删除"));
    add(2, Tr("Restore", "恢复"));
  } else if (id == ClearType) {
    const char *zh[] = {"植物", "僵尸", "梯子", "墓碑", "钉耙"};
    const char *en[] = {"Plant", "Zombie", "Ladder", "Grave", "Rake"};
    for (int i = 0; i < 5; ++i)
      add(i, Tr(en[i], zh[i]));
  } else if (id == Scene) {
    const char *zh[] = {"白天", "黑夜", "泳池", "雾夜", "屋顶", "月夜"};
    const char *en[] = {"Day", "Night", "Pool", "Fog", "Roof", "Moon"};
    for (int i = 0; i < 6; ++i)
      add(i, "[" + std::to_string(i) + "] " + Tr(en[i], zh[i]));
  } else if (id == Lily || id == Pot) {
    int n = id == Lily ? 8 : 7;
    for (int i = 0; i < n; ++i) {
      std::string s;
      int from = id == Lily ? lilyFrom[i] : potFrom[i],
          to = id == Lily ? lilyTo[i] : potTo[i];
      for (int col = 1; col <= 9; ++col)
        s += (col >= from && col <= to) ? "■" : "□";
      add(i, s);
    }
  } else if (id == GigaWeight)
    for (int i = 1000; i <= 6000; i += 100)
      add(i, std::to_string(i));
  else if (id == SpawnMode) {
    add(0, Tr("Natural (Game Generate)", "自然出怪（游戏生成）"));
    add(1, Tr("Extreme (Filling Evenly)", "极限出怪（均匀填充）"));
    add(2, Tr("Simulate (Weighted Random)", "模拟出怪（加权随机）"));
  } else if (id == Music) {
    const char *names[] = {
        "Grasswalk",     "Moongrains",      "Watery Graves",
        "Rigor Mormist", "Graze the Roof",  "Choose Your Seeds",
        "Crazy Dave",    "Zen Garden",      "Cerebrawl",
        "Loonboon",      "Ultimate Battle", "Brainiac Maniac"};
    for (int i = 0; i < 12; ++i)
      add(i, "[" + std::to_string(i + 1) + "] " + names[i]);
  } else if (id == Debug) {
    const char *en[] = {"Off", "Spawning", "Music", "Memory", "Collision"};
    const char *zh[] = {"关闭", "出怪", "音乐", "内存", "碰撞"};
    for (int i = 0; i < 5; ++i)
      add(i, Tr(en[i], zh[i]));
  } else if (id == Speed) {
    const char *names[] = {"10x", "5x", "2x", "1x", "0.5x", "0.2x", "0.1x"};
    for (int i = 0; i < 7; ++i)
      add(i, names[i]);
  } else if (id == Language) {
    add(0, "中文");
    add(1, "English");
  } else if (id == LineupFile)
    for (int i = 0; i < int(lineups.size()); ++i)
      add(i, lineups[i].first);
  return out;
}
void Panel::Refresh() {
  using namespace Classic;
  for (auto &[id, b] : buttons) {
    if (id >= 10 && id < 15) {
      static_cast<ClassicButton *>(b.get())->activeTab = id - 10 == page;
      continue;
    }
    if (id == Close)
      continue;
    auto s = std::find_if(layout.begin(), layout.end(),
                          [&](auto &spec) { return spec.id == id; });
    if (s == layout.end())
      continue;
    bool visible = s->page == page;
    if (id == SpawnMode)
      visible = false;
    if (id == LineupFile)
      visible = visible && !lineups.empty();
    if (id == LoadLineup)
      visible = visible && lineups.empty();
    b->SetVisible(visible);
    auto *c = static_cast<ClassicButton *>(b.get());
    c->checked = id >= FlagFirst
                     ? service.Get(static_cast<Flag>(id - FlagFirst))
                     : Value(id) != 0;
    if (id == Tooltips)
      c->checked = tooltips;
    if (id >= SpawnFirst && id < SpawnFirst + 20)
      b->mLabel = ShortZombie(spawnTypes[id - SpawnFirst], builtChinese);
    else if (s->kind == Kind::Choice && id != Mowers && id != Lily &&
             id != Pot) {
      auto opts = Options(id);
      for (auto &[v, name] : opts)
        if (v == Value(id))
          b->mLabel = name;
    }
    if (id == Generate) {
      auto opts = Options(SpawnMode);
      int v = Value(SpawnMode);
      b->mLabel = opts[std::clamp(v, 0, 2)].second;
    }
    bool disabled = service.state.replay;
    if (id == Adventure)
      disabled = disabled || Value(Mode) != 0;
    if (id == GigaWeight)
      disabled = disabled || Value(SpawnMode) != 2;
    if (id == FlagFirst + int(Flag::FreePlanting) ||
        id == FlagFirst + int(Flag::PlantInvincible) ||
        id == FlagFirst + int(Flag::PlantWeak) ||
        id == FlagFirst + int(Flag::Reload) ||
        id == FlagFirst + int(Flag::AutoCollect) ||
        id == FlagFirst + int(Flag::StopSpawning) ||
        id == FlagFirst + int(Flag::NoFog))
      disabled = disabled || quick;
    b->SetDisabled(disabled);
  }
  for (auto &[id, e] : edits) {
    auto spec = std::find_if(layout.begin(), layout.end(),
                             [&](auto &s) { return s.id == id; });
    if (spec != layout.end())
      e->SetVisible(spec->page == page);
    e->SetDisabled(service.state.replay);
  }
  if (detailsOverlay) {
    detailsOverlay->SetVisible(
        detailsOpen && embedded);
    if (detailsOverlay->mVisible)
      BringToFront(detailsOverlay.get());
  }
  MarkDirty();
}
void Panel::CloseMenu() {
  for (auto &b : popup) {
    RemoveWidget(b.get());
    retired.push_back(std::move(b));
  }
  popup.clear();
  choices.clear();
  menu = -1;
}
void Panel::OpenMenu(int id) {
  CloseMenu();
  choices = Options(id);
  if (choices.empty())
    return;
  menu = id;
  menuOffset = 0;
  for (int i = 0; i < int(choices.size()); ++i)
    if (choices[i].first == Value(id)) {
      menuOffset = std::max(0, i - 3);
      break;
    }
  BuildMenu();
}
void Panel::BuildMenu() {
  for (auto &b : popup) {
    RemoveWidget(b.get());
    retired.push_back(std::move(b));
  }
  popup.clear();
  if (menu < 0 || !buttons.contains(menu))
    return;
  auto *anchor = buttons[menu].get();
  int rowHeight = std::max(19, int(22 * scale));
  int count = std::min(8, int(choices.size()));
  menuOffset =
      std::clamp(menuOffset, 0, std::max(0, int(choices.size()) - count));
  int w = std::max(anchor->mWidth, std::min(mWidth - 12, int(260 * scale)));
  if (menu == Classic::Row || menu == Classic::Col || menu == Classic::Slot ||
      menu == Classic::Speed || menu == Classic::Language)
    w = std::max(anchor->mWidth, int(110 * scale));
  int x = std::clamp(anchor->mX, 4, std::max(4, mWidth - w - 4));
  int y = anchor->mY + anchor->mHeight;
  bool scroll = int(choices.size()) > count;
  int total = count * rowHeight + (scroll ? 2 * rowHeight : 0);
  if (y + total > mHeight - 4)
    y = std::max(4, anchor->mY - total);
  auto add = [&](int id, const std::string &label, int yy) {
    auto b = std::make_unique<ClassicButton>(id, this);
    b->mLabel = label;
    b->scale = scale;
    b->mLabelJustify = ButtonWidget::BUTTON_LABEL_LEFT;
    b->SetFont(font.get());
    b->Resize(x, yy, w, rowHeight);
    AddWidget(b.get());
    BringToFront(b.get());
    popup.push_back(std::move(b));
  };
  if (scroll) {
    add(MenuUp, "▲", y);
    y += rowHeight;
  }
  for (int i = 0; i < count; ++i) {
    auto label = choices[menuOffset + i].second;
    if (choices[menuOffset + i].first == Value(menu))
      label = "• " + label;
    add(MenuFirst + i, label, y + i * rowHeight);
  }
  if (scroll)
    add(MenuDown, "▼", y + count * rowHeight);
  MarkDirty();
}
void Panel::Choose(int index) {
  if (menu < 0 || index < 0 || index >= int(choices.size()))
    return;
  int id = menu, value = choices[index].first;
  CloseMenu();
  values[id] = value;
  using namespace Classic;
  if (id == Language) {
    if (value == 0 && !HasChineseFont()) {
      service.messageEn = "Supply trainer-font.ttf to enable Chinese.";
      values[id] = 1;
      return;
    }
    service.chinese = value == 0;
    service.SaveSettings();
    rebuild = true;
  } else if (id == Mowers)
    Submit(Action::Mowers, value);
  else if (id == Lily || id == Pot) {
    Submit(id == Lily ? Action::Lily : Action::Pot,
           id == Lily ? lilyFrom[value] : potFrom[value], 0, 0,
           id == Lily ? lilyTo[value] : potTo[value]);
  } else if (id == LineupFile && value < int(lineups.size())) {
    text[LineupText] = lineups[value].second;
    if (edits.contains(LineupText))
      edits[LineupText]->SetText(text[LineupText]);
  }
  Refresh();
}
void Panel::Submit(Action action, int value, int row, int col, int extra,
                   std::string payload) {
  service.Submit({action, value, row, col, extra, std::move(payload),
                  service.state.generation});
}
void Panel::PlaceCells(Action action, int type, int extra) {
  int row = Value(Classic::Row) - 1, col = Value(Classic::Col) - 1;
  for (int y = row < 0 ? 0 : row; y < (row < 0 ? service.state.rows : row + 1);
       ++y)
    for (int x = col < 0 ? 0 : col; x < (col < 0 ? 9 : col + 1); ++x)
      Submit(action, type, y, x, extra);
}
void Panel::LoadLineupFile(std::string path) {
  if (path.empty())
    return;
  std::ifstream in(Sexy::PathFromU8(path));
  if (!in) {
    service.messageEn = "Cannot read lineup file";
    service.messageZh = "无法读取阵型列表";
    return;
  }
  lineups.clear();
  std::string line;
  while (std::getline(in, line)) {
    if (line.size() > 32768)
      continue;
    auto colon = line.find(':');
    auto name = colon == std::string::npos ? std::to_string(lineups.size() + 1)
                                           : line.substr(0, colon);
    auto code = colon == std::string::npos ? line : line.substr(colon + 1);
    auto first = code.find_first_not_of(" \t\r\"'");
    if (first == std::string::npos)
      continue;
    code.erase(0, first);
    auto end = code.find_last_not_of(" \t\r\"'");
    code.resize(end + 1);
    try {
      Formats::DecodeLineup(code);
      lineups.emplace_back(name, code);
    } catch (...) {
    }
    if (lineups.size() >= 2000)
      break;
  }
  if (!lineups.empty()) {
    values[Classic::LineupFile] = 0;
    text[Classic::LineupText] = lineups.front().second;
    edits[Classic::LineupText]->SetText(text[Classic::LineupText]);
  } else {
    service.messageEn = "No compatible lineups in file";
    service.messageZh = "文件中没有兼容阵型";
  }
  Refresh();
}
void Panel::PackFiles(bool unpack) {
  using namespace Classic;
  auto source = Text(unpack ? File : Dir);
  if (source.empty())
    return;
  auto complete = [this, source, unpack](std::string dest) {
    if (dest.empty())
      return;
    if (unpack)
      dest += "/pvz-unpacked-" + std::to_string(SDL_GetTicks());
    Submit(unpack ? Action::Unpack : Action::Pack, 0, 0, 0, 0,
           source + "\n" + dest);
  };
#if defined(__ANDROID__) || defined(__IPHONEOS__)
  Trainer::ChooseFile(true, std::move(complete));
#else
  if (unpack)
    ChooseDirectory(std::move(complete));
  else
    Trainer::ChooseFile(true, std::move(complete));
#endif
}
void Panel::ButtonPress(int) {
  pressTicks = 0;
  ignoreGenerateRelease = false;
}
void Panel::ButtonDownTick(int id) {
  if (embedded && id == Classic::Generate && ++pressTicks == 40) {
    ignoreGenerateRelease = true;
    OpenMenu(Classic::SpawnMode);
  }
}
void Panel::ButtonDepress(int id) {
  if (id == Classic::Generate && ignoreGenerateRelease) {
    ignoreGenerateRelease = false;
    return;
  }
  using namespace Classic;
  if (id >= 10 && id < 15) {
    page = id - 10;
    CloseMenu();
    Refresh();
    return;
  }
  if (id == MenuUp || id == MenuDown) {
    menuOffset += (id == MenuUp ? -5 : 5);
    BuildMenu();
    return;
  }
  if (id >= MenuFirst && id < MenuFirst + 8) {
    Choose(menuOffset + id - MenuFirst);
    return;
  }
  if (menu >= 0 && id != Close) {
    CloseMenu();
    return;
  }
  if (id == Close) {
    close();
    return;
  }
  if (service.state.replay)
    return;
  if (id >= FlagFirst && id < FlagFirst + int(Flag::Count)) {
    auto flag = static_cast<Flag>(id - FlagFirst);
    bool on = !service.Get(flag);
    service.Set(flag, on);
    if (flag == Flag::FreePlanting)
      service.Set(Flag::NoCooldown, on);
    Refresh();
    return;
  }
  if (id >= SpawnFirst && id < SpawnFirst + 20) {
    values[id] = !Value(id);
    Refresh();
    return;
  }

  auto spec = std::find_if(layout.begin(), layout.end(),
                           [&](auto &s) { return s.id == id; });
  if (spec != layout.end() && spec->kind == Kind::Choice) {
    OpenMenu(id);
    return;
  }
  try {
    switch (id) {
    case SetSun:
      Submit(Action::Sun, Number(SunInput));
      break;
    case SetMoney: {
      int money = Number(MoneyInput);
      if (money < 0 || money > 99999)
        throw std::runtime_error(Tr("Wallet must be 0..99999 (x10 coins).",
                                    "钱包数值为 0..99999（每单位 10 金币）。"));
      Submit(Action::Money, money * 10);
      break;
    }
    case SetTree:
      Submit(Action::TreeHeight, Number(TreeInput));
      break;
    case Mix:
      Submit(Action::MixMode, Value(Mode), 0, 0, Value(Adventure) + 1);
      break;
    case SetRounds:
      Submit(Action::Endless, Number(Rounds));
      break;
    case Unlock:
      Submit(Action::Unlock);
      break;
    case Win:
      Submit(Action::Win, 0, 0, 0, Value(BrightestCob));
      break;
    case BrightestCob:
    case Imitater:
    case SeedImitater:
    case GigaLimit:
      values[id] = !Value(id);
      break;
    case PutPlant:
      PlaceCells(Action::Plant, Value(Plant), Value(Imitater));
      break;
    case PutZombie:
      PlaceCells(Action::Zombie, Value(Zombie));
      break;
    case Grave:
      PlaceCells(Action::Grave);
      break;
    case Ladder:
      PlaceCells(Action::Ladder);
      break;
    case Rake:
      PlaceCells(Action::Rake);
      break;
    case Clear: {
      int v = Value(ClearType);
      if (v < 2)
        Submit(v == 0 ? Action::ClearPlants : Action::ClearZombies);
      else {
        int types[] = {3, 1, 11};
        Submit(Action::ClearItems, types[std::clamp(v - 2, 0, 2)]);
      }
      break;
    }
    case SetSlot:
      Submit(Action::Slot, Value(Seed), 0, Value(Slot), Value(SeedImitater));
      break;
    case AutoLadder:
      Submit(Action::AutoLadder);
      break;
    case Quick: {
      quick = !quick;
      values[id] = quick;
      Flag set[] = {Flag::AutoCollect,     Flag::FreePlanting, Flag::NoCooldown,
                    Flag::PlantInvincible, Flag::PlantWeak,    Flag::Reload,
                    Flag::StopSpawning,    Flag::NoFog};
      for (auto f : set) {
        if (quick) {
          quickSaved[int(f)] = service.Get(f);
          service.Set(f, f != Flag::PlantWeak);
        } else
          service.Set(f, quickSaved[int(f)]);
      }
      // Portable additionally supports direct lawn placement without leaving
      // this mode. Its selected plant follows the original battlefield
      // selector.
      if (quick && place)
        place({Action::Plant,
               Value(Plant),
               0,
               0,
               Value(Imitater),
               {},
               service.state.generation});
      break;
    }
    case ResetScene:
      Submit(Action::Scene, Value(Scene));
      break;
    case LoadLineup:
      Trainer::ChooseFile(
          false, [this](std::string p) { LoadLineupFile(std::move(p)); });
      break;
    case GetLineup:
      Submit(Action::ExportLineup);
      break;
    case CopyLineup:
      if (mWidgetManager)
        mWidgetManager->mApp->CopyToClipboard(Text(LineupText));
      break;
    case PasteLineup:
      if (mWidgetManager) {
        auto s = mWidgetManager->mApp->GetClipboard();
        edits[LineupText]->SetText(s);
        text[LineupText] = std::move(s);
      }
      break;
    case ApplyLineup:
      Submit(Action::ImportLineup, 0, 0, 0, 0, Text(LineupText));
      break;
    case Screenshot:
      Submit(Action::Screenshot);
      break;
    case Generate: {
      std::string types = "0";
      for (int i = 0; i < 20; ++i)
        if (Value(SpawnFirst + i))
          types += ' ' + std::to_string(spawnTypes[i]);
      Submit(Action::SpawnGenerate, Value(SpawnMode), Value(GigaLimit),
             Value(GigaWeight), 50, types);
      break;
    }
    case Details:
      Submit(Action::SpawnRead);
      about = false;
      detailsOpen = true;
      break;
    case PlayMusic:
      Submit(Action::Music, Value(Music) + 1);
      break;
    case SaveFolder:
      Submit(Action::SaveFolder);
      break;
    case Classic::ChooseFile:
      Trainer::ChooseFile(false, [this](std::string p) {
        if (!p.empty()) {
          text[File] = p;
          if (edits.contains(File))
            edits[File]->SetText(p);
        }
      });
      break;
    case ChooseDir:
      ChooseDirectory([this](std::string p) {
        if (!p.empty()) {
          text[Dir] = p;
          if (edits.contains(Dir))
            edits[Dir]->SetText(p);
        }
      });
      break;
    case Pack:
      PackFiles(false);
      break;
    case Unpack:
      PackFiles(true);
      break;
    case SetDebug:
      Submit(Action::Debug, Value(Debug));
      break;
    case SetSpeed:
      Submit(Action::Speed, speedValues[std::clamp(Value(Speed), 0, 6)]);
      break;
    case Documentation:
      Submit(Action::Documentation);
      break;
    case About:
      Submit(Action::About);
      about = true;
      detailsOpen = true;
      break;
    case Tooltips:
      tooltips = !tooltips;
      break;
    }
  } catch (const std::exception &e) {
    service.messageEn = e.what();
    service.messageZh = e.what();
  }
  Refresh();
}
void Panel::EditWidgetText(int id, const std::string &) {
  using namespace Classic;
  if (id == SunInput)
    ButtonDepress(SetSun);
  else if (id == MoneyInput)
    ButtonDepress(SetMoney);
  else if (id == TreeInput)
    ButtonDepress(SetTree);
  else if (id == Rounds)
    ButtonDepress(SetRounds);
}
void Panel::MouseWheel(int delta) {
  if (menu >= 0) {
    menuOffset -= delta * 2;
    BuildMenu();
  }
}
void Panel::KeyDown(KeyCode key) {
  if (key == KEYCODE_ESCAPE) {
    if (menu >= 0)
      CloseMenu();
    else if (detailsOpen)
      detailsOpen = false;
    else
      close();
  }
}
void Panel::MouseDown(int, int, int button, int) {
  if (button == -1 && page == 3)
    OpenMenu(Classic::SpawnMode);
  else if (menu >= 0)
    CloseMenu();
}
void Panel::ButtonMouseEnter(int id) {
  hover = id;
  hoverSince = SDL_GetTicks();
}
void Panel::ButtonMouseLeave(int id) {
  if (hover == id)
    hover = -1;
}
std::string Panel::Tip(int id) const {
  bool english = !service.chinese || tooltips;
  if (id >= Classic::FlagFirst) {
    auto f = static_cast<Flag>(id - Classic::FlagFirst);
    for (auto &feature : Features())
      if (feature.flag == int(f))
        return english ? feature.hintEn : feature.hintZh;
  }
  if (id == Classic::Language)
    return "中文 / English";
  if (id == Classic::Generate)
    return english ? "Right-click / hold for Natural / Extreme / Simulate"
                   : "右键或长按选择：自然出怪／极限出怪／模拟出怪";
  if (id == Classic::Quick)
    return english ? "Quick setup and continuous lawn placement"
                   : "快捷布阵，点击战场连续放置";
  auto i = buttons.find(id);
  return i == buttons.end() ? "" : i->second->mLabel;
}
void Panel::Update() {
  retired.clear();
  Widget::Update();
  if (builtChinese != service.chinese)
    rebuild = true;
  if (rebuild)
    Build();
  if (lastServiceText != service.text) {
    lastServiceText = service.text;
    if (page == 2 && edits.contains(Classic::LineupText)) {
      edits[Classic::LineupText]->SetText(service.text);
      text[Classic::LineupText] = service.text;
    }
  }
  Refresh();
}
void Panel::Draw(Graphics *g) {
  g->SetColor(Color(243, 243, 243));
  g->FillRect(0, 0, mWidth, mHeight);
  g->SetFont(font.get());
  auto rect = [&](int x, int y, int w, int h) {
    return Rect(originX + std::lround(x * scale),
                originY + std::lround(y * scale), std::lround(w * scale),
                std::lround(h * scale));
  };
  int m = builtChinese ? 10 : 8, th = builtChinese ? 30 : 27;
  auto group =
      rect(m, m + th, PreferredWidth() - 2 * m, PreferredHeight() - 2 * m - th);
  g->SetColor(Color(218, 218, 218));
  g->DrawRect(group.mX, group.mY, group.mWidth - 1, group.mHeight - 1);
  auto textAt = [&](std::string_view s, int x, int y) {
    g->SetColor(Color::Black);
    g->DrawString(s, originX + x * scale, originY + y * scale);
  };
  if (font) {
    std::string status =
        service.state.board
            ? Tr("PvZ Portable - In game", "PvZ Portable · 关卡中")
            : Tr("PvZ Portable - Ready", "PvZ Portable · 就绪");
    int sx = builtChinese ? 230 : 330;
    textAt(status, sx, m + 20);
    for (auto &spec : layout)
      if (spec.page == page && spec.kind == Classic::Kind::Label)
        textAt(spec.label, spec.x,
               spec.y + (spec.h - font->GetHeight() / scale) / 2 +
                   font->GetAscent() / scale);
  }
  if (menu >= 0 && !popup.empty()) {
    auto &first = popup.front();
    auto &last = popup.back();
    g->SetColor(Color(105, 105, 105));
    g->FillRect(first->mX - 1, first->mY - 1, first->mWidth + 2,
                last->mY + last->mHeight - first->mY + 2);
  }
  if (hover >= 0 && menu < 0 && SDL_GetTicks() - hoverSince > 650 && font) {
    auto tip = Tip(hover);
    if (!tip.empty()) {
      int width = std::min(mWidth - 12, font->StringWidth(tip) + 12);
      auto t = rect(8, PreferredHeight() - 27, 1, 1);
      g->SetColor(Color(255, 255, 225));
      g->FillRect(t.mX, t.mY, width, int(23 * scale));
      g->SetColor(Color(130, 130, 110));
      g->DrawRect(t.mX, t.mY, width - 1, int(23 * scale) - 1);
      g->SetColor(Color::Black);
      while (!tip.empty() && font->StringWidth(tip) > width - 10) {
        auto p = UTF8PrevBoundary(tip, tip.size());
        tip.resize(p);
      }
      g->DrawString(tip, t.mX + 4, t.mY + font->GetAscent() + 3);
    }
  }
}
void Panel::ScrollDetails(int dx, int dy) {
  detailsCol = std::max(0, detailsCol + dx);
  detailsRow = std::clamp(detailsRow + dy, 0, 32);
}
void Panel::DetailsClick(int x, int y, int button) {
  if (embedded && x > mWidth - 55 && y < 28) {
    detailsOpen = false;
    Refresh();
    return;
  }
  if (button < 0) {
    detailsMenu = !detailsMenu;
    MarkDirty();
    return;
  }
  if (detailsMenu) {
    int operation = (y - 27) / 25;
    if (x >= 5 && x < 145 && y >= 27 && operation >= 0 && operation < 3) {
      if (operation == 0)
        Submit(Action::SpawnRead);
      else
        Trainer::ChooseFile(
            operation == 1, [this, operation](std::string path) {
              if (!path.empty())
                Submit(operation == 1 ? Action::SpawnSave : Action::SpawnLoad,
                       0, 0, 0, 0, std::move(path));
            });
    }
    detailsMenu = false;
    MarkDirty();
    return;
  }
  if (x < 180 && y < 28)
    Submit(Action::SpawnRead);
}
void Panel::DrawDetails(Graphics *g, int width, int height) {
  if (!font)
    return;
  g->SetFont(font.get());
  g->SetColor(Color(243, 243, 243));
  g->FillRect(0, 0, width, height);
  g->SetColor(Color::Black);
  if (about) {
    g->DrawString(
        Tr("About PvZ Toolkit (Portable)", "关于 PvZ Toolkit（Portable）"), 12,
        24);
    std::string current;
    int y = 55;
    for (size_t i = 0; i < service.text.size();) {
      auto next = UTF8NextBoundary(service.text, i);
      auto cp = service.text.substr(i, next - i);
      i = next;
      if (cp == "\n" || font->StringWidth(current + cp) > width - 30) {
        g->DrawString(current, 12, y);
        current.clear();
        y += font->GetHeight() + 4;
        if (y > height - 15)
          break;
      }
      if (cp != "\n")
        current += cp;
    }
    if (!current.empty() && y < height)
      g->DrawString(current, 12, y);
    if (embedded)
      g->DrawString("×", width - 30, 22);
    return;
  }
  std::vector<std::vector<int>> waves;
  std::istringstream stream(service.text);
  std::string line;
  try {
    while (std::getline(stream, line)) {
      auto v = Formats::ParseIntegers(line, 0, 32, 50);
      if (!v.empty())
        waves.push_back(std::move(v));
    }
  } catch (...) {
    return;
  }
  const int nameWidth = builtChinese ? 75 : 175, cw = 36,
            rh = std::max(18, font->GetHeight() + 1), header = 21;
  int cols = std::max(1, (width - nameWidth - 10) / cw),
      rows = std::max(1, (height - header - 12) / rh);
  detailsCol =
      std::clamp(detailsCol, 0, std::max(0, int(waves.size()) + 1 - cols));
  for (int r = -1; r < std::min(rows, 33 - detailsRow); ++r) {
    int y = r < 0 ? 5 : 5 + header + r * rh, h = r < 0 ? header : rh;
    for (int c = -1; c < cols; ++c) {
      int x = c < 0 ? 5 : 5 + nameWidth + c * cw, w = c < 0 ? nameWidth : cw;
      int wave = c + detailsCol;
      bool total = wave == int(waves.size());
      g->SetColor(r < 0   ? Color(226, 226, 226)
                  : total ? Color(234, 243, 249)
                          : Color(250, 250, 250));
      g->FillRect(x, y, w, h);
      g->SetColor(Color(178, 178, 178));
      g->DrawRect(x, y, w, h);
      g->SetColor(Color::Black);
      std::string value;
      if (r < 0)
        value = c < 0   ? "···"
                : total ? Tr("Total", "总计")
                        : std::to_string(wave + 1);
      else if (c < 0)
        value = Tr(ZombieNames[r + detailsRow].first,
                   ZombieNames[r + detailsRow].second);
      else if (total) {
        int n = 0;
        for (auto &list : waves)
          n += std::count(list.begin(), list.end(), r + detailsRow);
        value = std::to_string(n);
      } else if (wave < int(waves.size()))
        value = std::to_string(
            std::count(waves[wave].begin(), waves[wave].end(), r + detailsRow));
      while (!value.empty() && font->StringWidth(value) > w - 6)
        value.resize(UTF8PrevBoundary(value, value.size()));
      g->DrawString(value, x + std::max(3, (w - font->StringWidth(value)) / 2),
                    y + (h - font->GetHeight()) / 2 + font->GetAscent());
    }
  }
  if (embedded)
    g->DrawString("×", width - 25, 22);
  if (detailsMenu) {
    const char *en[] = {"Refresh", "Save", "Load"};
    const char *zh[] = {"刷新", "保存", "加载"};
    for (int i = 0; i < 3; ++i) {
      g->SetColor(Color(243, 243, 243));
      g->FillRect(5, 27 + i * 25, 140, 25);
      g->SetColor(Color(140, 140, 140));
      g->DrawRect(5, 27 + i * 25, 140, 25);
      g->SetColor(Color::Black);
      g->DrawString(Tr(en[i], zh[i]), 12, 45 + i * 25);
    }
  }
}
} // namespace Trainer
#endif
