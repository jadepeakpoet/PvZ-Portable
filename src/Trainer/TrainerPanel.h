// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#ifdef PVZ_TRAINER
#include "Trainer.h"
#include "TrainerClassicLayout.h"
#include "widget/ButtonListener.h"
#include "widget/EditListener.h"
#include "widget/Widget.h"
#include <functional>
#include <map>
#include <memory>
namespace Sexy {
class ButtonWidget;
class EditWidget;
} // namespace Sexy
namespace Trainer {
class Panel : public Sexy::Widget,
              public Sexy::ButtonListener,
              public Sexy::EditListener {
public:
  bool embedded = false;
  int pressTicks = 0;
  bool ignoreGenerateRelease = false;
  Panel(Service &, std::function<void()> close,
        std::function<void(Command)> place = {});
  ~Panel() override;
  void Resize(int, int, int, int) override;
  void Draw(Sexy::Graphics *) override;
  void Update() override;
  void MouseWheel(int) override;
  void KeyDown(Sexy::KeyCode) override;
  void MouseDown(int, int, int, int) override;
  void ButtonDepress(int) override;
  void ButtonPress(int) override;
  void ButtonDownTick(int) override;
  void ButtonMouseEnter(int) override;
  void ButtonMouseLeave(int) override;
  void ButtonMouseMove(int, int, int) override {}
  void EditWidgetText(int, const std::string &) override;
  void Refresh();
  void SetValue(int, int);
  int Value(int) const;
  int PreferredWidth() const;
  int PreferredHeight() const;
  bool detailsOpen = false;
  void DrawDetails(Sexy::Graphics *, int width, int height);
  void ScrollDetails(int dx, int dy);
  void DetailsClick(int x, int y, int button);

private:
  Service &service;
  std::function<void()> close;
  std::function<void(Command)> place;
  std::unique_ptr<Sexy::_Font> font;
  std::unique_ptr<Sexy::Widget> detailsOverlay;
  std::vector<Classic::Control> layout;
  std::map<int, std::unique_ptr<Sexy::ButtonWidget>> buttons;
  std::map<int, std::unique_ptr<Sexy::EditWidget>> edits;
  std::map<int, int> values;
  std::map<int, std::string> text;
  std::vector<std::unique_ptr<Sexy::ButtonWidget>> popup;
  std::vector<std::unique_ptr<Sexy::ButtonWidget>> retired;
  std::vector<std::pair<int, std::string>> choices;
  std::vector<std::pair<std::string, std::string>> lineups;
  std::array<bool, static_cast<int>(Flag::Count)> quickSaved{};
  bool builtChinese = true, rebuild = true, quick = false, tooltips = false;
  bool about = false, detailsMenu = false;
  int page = 3, menu = -1, menuOffset = 0, hover = -1, detailsRow = 0,
      detailsCol = 0;
  double scale = 1;
  int originX = 0, originY = 0;
  std::string lastServiceText;
  unsigned hoverSince = 0;
  const char *Tr(const char *, const char *) const;
  void Build();
  void Relayout();
  void CloseMenu();
  void OpenMenu(int);
  void BuildMenu();
  void Choose(int);
  std::vector<std::pair<int, std::string>> Options(int) const;
  void Submit(Action, int value = 0, int row = 0, int col = 0, int extra = 0,
              std::string payload = {});
  void PlaceCells(Action, int type = 0, int extra = 0);
  void LoadLineupFile(std::string);
  void PackFiles(bool unpack);
  int Number(int) const;
  std::string Text(int) const;
  std::string Tip(int) const;
};
} // namespace Trainer
#endif
