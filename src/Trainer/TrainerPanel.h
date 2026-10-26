// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#ifdef PVZ_TRAINER
#include "Trainer.h"
#include "widget/ButtonListener.h"
#include "widget/EditListener.h"
#include "widget/Widget.h"
#include <functional>
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
  Panel(Service &service, std::function<void()> close,
        std::function<void(Command)> place = {});
  ~Panel() override;
  void Resize(int x, int y, int w, int h) override;
  void Draw(Sexy::Graphics *g) override;
  void Update() override;
  void MouseWheel(int delta) override;
  void ButtonDepress(int id) override;
  void ButtonPress(int) override {}
  void ButtonDownTick(int) override {}
  void ButtonMouseEnter(int) override {}
  void ButtonMouseLeave(int) override {}
  void ButtonMouseMove(int, int, int) override {}
  void EditWidgetText(int, const std::string &) override;
  void Refresh();

private:
  Service &service;
  std::function<void()> close;
  std::function<void(Command)> place;
  std::unique_ptr<Sexy::_Font> font, titleFont;
  std::vector<std::unique_ptr<Sexy::ButtonWidget>> buttons;
  std::array<std::unique_ptr<Sexy::EditWidget>, 5> edits;
  std::vector<int> indices;
  int page = 0, offset = 0, selected = -1, listRows = 8;
  bool needsRefresh = true, lastChinese = true;
  bool choosingTypes = false;
  Snapshot previousState;
  bool previousPaused = false;
  int featureOffset = 0;
  std::string lastText;
  const char *Tr(const char *en, const char *zh) const;
  void Select(int index);
  void Execute();
  Command ReadCommand(bool &valid);
  void Wrap(Sexy::Graphics *g, std::string_view text, int x, int y, int width,
            int maxLines);
};
} // namespace Trainer
#endif
