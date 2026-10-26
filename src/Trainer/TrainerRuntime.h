// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#ifdef PVZ_TRAINER
#include <cstdint>
#include <memory>
#include <string>
union SDL_Event;
class LawnApp;
namespace Sexy {
class SexyAppBase;
}
namespace Trainer {
class Service;
class Panel;
class Runtime {
public:
  explicit Runtime(LawnApp *app);
  ~Runtime();
  void Tick();
  void Pump();
  void Toggle();
  void InvalidateBoard();
  int UpdateCount(int original);
  Service &GetService();
  Panel &GetPanel();
  std::uint32_t WindowId() const;
  bool SavePanelImage(const std::string &path);
  bool Event(const SDL_Event &event);
  bool ToolFocused() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl;
};
bool HandleEvent(Sexy::SexyAppBase *app, const SDL_Event &event);
void Pump(Sexy::SexyAppBase *app);
bool HasToolFocus(Sexy::SexyAppBase *app);
void BeforePresent(Sexy::SexyAppBase *app);
} // namespace Trainer
#endif
