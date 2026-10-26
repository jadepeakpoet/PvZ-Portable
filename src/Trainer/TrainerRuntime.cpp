// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef PVZ_TRAINER
#include "TrainerRuntime.h"
#include "Lawn/Board.h"
#include "LawnApp.h"
#include "Trainer.h"
#include "TrainerFiles.h"
#include "TrainerFont.h"
#include "TrainerPanel.h"
#include "graphics/Font.h"
#include "graphics/GLPlatform.h"
#include "graphics/MemoryImage.h"
#include "imagelib/ImageLib.h"
#include "misc/KeyCodes.h"
#include "widget/ButtonListener.h"
#include "widget/ButtonWidget.h"
#include "widget/WidgetManager.h"
#include <SDL.h>
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace Trainer {
namespace {
Runtime *active = nullptr;
LawnApp *owner = nullptr;
Sexy::KeyCode Key(SDL_Keycode k) {
  using namespace Sexy;
  if (k >= SDLK_a && k <= SDLK_z)
    return static_cast<KeyCode>('A' + k - SDLK_a);
  if (k >= SDLK_0 && k <= SDLK_9)
    return static_cast<KeyCode>('0' + k - SDLK_0);
  switch (k) {
  case SDLK_BACKSPACE:
    return KEYCODE_BACK;
  case SDLK_DELETE:
    return KEYCODE_DELETE;
  case SDLK_LEFT:
    return KEYCODE_LEFT;
  case SDLK_RIGHT:
    return KEYCODE_RIGHT;
  case SDLK_UP:
    return KEYCODE_UP;
  case SDLK_DOWN:
    return KEYCODE_DOWN;
  case SDLK_HOME:
    return KEYCODE_HOME;
  case SDLK_END:
    return KEYCODE_END;
  case SDLK_RETURN:
  case SDLK_KP_ENTER:
    return KEYCODE_RETURN;
  case SDLK_LSHIFT:
  case SDLK_RSHIFT:
    return KEYCODE_SHIFT;
  case SDLK_LCTRL:
  case SDLK_RCTRL:
  case SDLK_LGUI:
  case SDLK_RGUI:
    return KEYCODE_CONTROL;
  case SDLK_TAB:
    return KEYCODE_TAB;
  case SDLK_ESCAPE:
    return KEYCODE_ESCAPE;
  default:
    return KEYCODE_UNKNOWN;
  }
}
class Launcher : public Sexy::ButtonWidget, public Sexy::ButtonListener {
public:
  Runtime &runtime;
  Launcher(Runtime &r) : ButtonWidget(1, this), runtime(r) {
    auto font = CreateFont(16);
    if (font)
      SetFont(font.get());
    mLabel = "Tools / 工具";
    SetBkgColor(Sexy::Color(75, 98, 63));
    SetLabelColor(Sexy::Color::White);
  }
  void ButtonDepress(int) override { runtime.Toggle(); }
  void ButtonPress(int) override {}
  void ButtonDownTick(int) override {}
  void ButtonMouseEnter(int) override {}
  void ButtonMouseLeave(int) override {}
  void ButtonMouseMove(int, int, int) override {}
};
class PlacementPreview : public Sexy::Widget {
  Service &service;
  std::unique_ptr<Sexy::_Font> font;

public:
  int row = -1, col = -1;
  Command command;
  explicit PlacementPreview(Service &s) : service(s), font(CreateFont(17)) {
    mMouseVisible = false;
    SetVisible(false);
  }
  void Draw(Sexy::Graphics *g) override {
    g->SetColor(Sexy::Color(24, 36, 29, 235));
    g->FillRect(130, 5, mWidth - 138, 38);
    if (font) {
      g->SetFont(font.get());
      g->SetColor(Sexy::Color(238, 239, 213));
      auto text = service.chinese
                      ? "连续放置：点击格子；右侧完成"
                      : "Place repeatedly: click a cell; Done to finish";
      g->DrawString(text, 140, 30);
      g->DrawString(service.chinese ? "完成" : "Done", mWidth - 65, 30);
    }
    if (row >= 0 && row < service.state.rows && col >= 0 && col < 9 &&
        service.app->mBoard) {
      auto *b = service.app->mBoard;
      int x = b->GridToPixelX(col, row), y = b->GridToPixelY(col, row);
      g->SetColor(Sexy::Color(120, 235, 130, 80));
      g->FillRect(x, y, 80, 85);
      g->SetColor(Sexy::Color(170, 255, 160));
      g->DrawRect(x, y, 79, 84);
    }
  }
};
} // namespace
struct Runtime::Impl {
  LawnApp *app;
  Service service;
  std::unique_ptr<Panel> panel;
  std::unique_ptr<Launcher> launcher;
  std::unique_ptr<PlacementPreview> preview;
  std::unique_ptr<Sexy::WidgetManager> manager;
  std::unique_ptr<Sexy::MemoryImage> canvas;
  SDL_Window *window = nullptr;
  bool mobile = false, visible = false, closeRequested = false,
       placementRequested = false;
  Command placement;
  std::uint32_t lastDraw = 0;
  double credit = 0;
  int x = SDL_WINDOWPOS_CENTERED, y = SDL_WINDOWPOS_CENTERED, w = 960, h = 700;
  std::string settings;
  Impl(LawnApp *a) : app(a), service(a) {
#if defined(__ANDROID__) || defined(__IPHONEOS__) ||                           \
    defined(__EMSCRIPTEN__) || defined(__SWITCH__)
    mobile = true;
#endif
    mobile = mobile || a->mTrainerMobile;
    settings = Sexy::GetAppDataPath("trainer-window.cfg");
    std::ifstream in(settings);
    int px, py, pw, ph;
    if (in >> px >> py >> pw >> ph && pw >= 800 && pw <= 2400 && ph >= 640 &&
        ph <= 1600) {
      x = px;
      y = py;
      w = pw;
      h = ph;
    }
    panel = std::make_unique<Panel>(
        service, [this] { closeRequested = true; },
        [this](Command c) {
          placement = std::move(c);
          placementRequested = true;
        });
    panel->SetVisible(false);
  }
  void SaveWindow() {
    if (!window)
      return;
    SDL_GetWindowPosition(window, &x, &y);
    SDL_GetWindowSize(window, &w, &h);
    std::ofstream out(settings);
    out << x << ' ' << y << ' ' << w << ' ' << h << '\n';
  }
};
Runtime::Runtime(LawnApp *app) : impl(std::make_unique<Impl>(app)) {
  active = this;
  owner = app;
  impl->launcher = std::make_unique<Launcher>(*this);
  impl->launcher->Resize(8, 8, 115, 32);
  app->mWidgetManager->AddWidget(impl->launcher.get());
  impl->preview = std::make_unique<PlacementPreview>(impl->service);
  impl->preview->Resize(0, 0, app->mWidth, app->mHeight);
  app->mWidgetManager->AddWidget(impl->preview.get());
  if (impl->mobile) {
    impl->panel->Resize(10, 10, app->mWidth - 20, app->mHeight - 20);
    app->mWidgetManager->AddWidget(impl->panel.get());
  }
  if (app->mTrainerInitiallyOpen)
    Toggle();
}
Runtime::~Runtime() {
  auto &p = *impl;
  p.SaveWindow();
  CancelFileDialogs();
  p.app->mWidgetManager->RemoveWidget(p.launcher.get());
  p.app->mWidgetManager->RemoveWidget(p.preview.get());
  if (p.mobile)
    p.app->mWidgetManager->RemoveWidget(p.panel.get());
  else if (p.manager)
    p.manager->RemoveWidget(p.panel.get());
  p.panel.reset();
  p.manager.reset();
  p.canvas.reset();
  if (p.window)
    SDL_DestroyWindow(p.window);
  active = nullptr;
  owner = nullptr;
}
Service &Runtime::GetService() { return impl->service; }
Panel &Runtime::GetPanel() { return *impl->panel; }
std::uint32_t Runtime::WindowId() const {
  return impl->window ? SDL_GetWindowID(impl->window) : 0;
}
bool Runtime::SavePanelImage(const std::string &path) {
  auto &p = *impl;
  Sexy::MemoryImage canvas;
  canvas.Create(p.panel->mWidth, p.panel->mHeight);
  Sexy::Graphics g(&canvas);
  Sexy::ModalFlags flags;
  p.panel->mWidgetManager->InitModalFlags(&flags);
  p.panel->DrawAll(&flags, &g);
  ImageLib::Image image;
  image.mWidth = canvas.mWidth;
  image.mHeight = canvas.mHeight;
  image.mBits = std::make_unique<uint32_t[]>(static_cast<size_t>(image.mWidth) *
                                             image.mHeight);
  std::copy_n(canvas.GetBits(),
              static_cast<size_t>(image.mWidth) * image.mHeight,
              image.mBits.get());
  return ImageLib::WritePNGImage(path, &image);
}
void Runtime::InvalidateBoard() {
  impl->service.InvalidateBoard();
  impl->credit = 0;
  if (impl->preview)
    impl->preview->SetVisible(false);
}
void Runtime::Toggle() {
  auto &p = *impl;
  if (!p.visible && p.app->IsInDemoMode()) {
    p.service.messageEn = "Tools are unavailable during recording / replay.";
    p.service.messageZh = "录制或回放期间不能打开工具箱。";
    return;
  }
  if (!p.mobile && !p.window) {
    // A software surface keeps the game's GL context and texture cache
    // untouched.
    p.window =
        SDL_CreateWindow("PvZ Toolkit / 工具箱", p.x, p.y, p.w, p.h,
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!p.window) {
      p.service.messageEn = SDL_GetError();
      p.service.messageZh = p.service.messageEn;
      p.mobile = true;
      p.panel->Resize(10, 10, p.app->mWidth - 20, p.app->mHeight - 20);
      p.app->mWidgetManager->AddWidget(p.panel.get());
    } else {
      SDL_SetWindowMinimumSize(p.window, 800, 640);
      SDL_Rect bounds{};
      if (SDL_GetDisplayUsableBounds(
              SDL_GetWindowDisplayIndex(
                  static_cast<SDL_Window *>(p.app->mWindow)),
              &bounds) == 0 &&
          (p.x < bounds.x - p.w + 80 || p.y < bounds.y ||
           p.x > bounds.x + bounds.w - 80 || p.y > bounds.y + bounds.h - 80))
        SDL_SetWindowPosition(p.window, SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED);
      p.manager = std::make_unique<Sexy::WidgetManager>(p.app);
      p.manager->AddWidget(p.panel.get());
      p.canvas = std::make_unique<Sexy::MemoryImage>();
    }
  }
  p.visible = !p.visible;
  p.service.panelOpen = p.visible;
  p.service.mobilePause = p.mobile && p.visible;
  p.panel->SetVisible(p.visible);
  if (p.mobile) {
    if (p.visible)
      p.app->mWidgetManager->AddBaseModal(p.panel.get());
    else
      p.app->mWidgetManager->RemoveBaseModal(p.panel.get());
    p.app->mWidgetManager->MarkAllDirty();
  } else {
    if (p.visible) {
      SDL_ShowWindow(p.window);
      SDL_RaiseWindow(p.window);
    } else {
      p.SaveWindow();
      SDL_HideWindow(p.window);
      p.manager->LostFocus();
    }
  }
}
void Runtime::Tick() {
  auto &p = *impl;
  p.service.Tick();
  if (p.mobile && p.visible && p.service.screenshotRequested)
    Toggle();
  p.launcher->SetVisible(!p.service.state.replay);
  if (p.service.state.replay) {
    if (p.visible) {
      Toggle();
      SDL_RaiseWindow(static_cast<SDL_Window *>(p.app->mWindow));
    }
    p.preview->SetVisible(false);
  }
  p.launcher->mLabel = p.service.chinese ? "工具箱" : "Toolkit";
  p.launcher->MarkDirty();
  p.app->mWidgetManager->BringToFront(p.launcher.get());
  if (p.mobile && p.visible)
    p.app->mWidgetManager->BringToFront(p.panel.get());
  Pump();
}
int Runtime::UpdateCount(int original) {
  auto &p = *impl;
  if (!p.service.state.board || p.service.state.replay)
    return original;
  if (p.service.paused || p.service.mobilePause)
    return 0;
  if (p.app->mGameScene != GameScenes::SCENE_PLAYING ||
      p.app->GetDialogCount() > 0)
    return original;
  p.credit += std::clamp(p.service.speed, 0.05, 20.0) * original;
  int n = static_cast<int>(p.credit);
  p.credit -= n;
  return n;
}
void Runtime::Pump() {
  auto &p = *impl;
  PumpFileDialogs();
  if (p.placementRequested) {
    p.placementRequested = false;
    if (p.service.state.board && !p.service.state.replay) {
      p.preview->command = p.placement;
      p.preview->SetVisible(true);
      p.app->mWidgetManager->BringToFront(p.preview.get());
      if (p.mobile && p.visible)
        Toggle();
      SDL_RaiseWindow(static_cast<SDL_Window *>(p.app->mWindow));
      p.service.messageEn =
          "Placement mode: click lawn cells. Right-click / Done finishes.";
      p.service.messageZh = "连续放置：点击战场格子；右键或点击完成退出。";
    }
  }
  if (p.closeRequested) {
    p.closeRequested = false;
    if (p.visible)
      Toggle();
  }
  if (!p.visible)
    return;
  std::uint32_t now = SDL_GetTicks();
  if (now - p.lastDraw < 33)
    return;
  p.lastDraw = now;
  if (p.mobile) {
    p.panel->Update();
    return;
  }
  SDL_GetWindowSize(p.window, &p.w, &p.h);
  if (p.canvas->mWidth != p.w || p.canvas->mHeight != p.h) {
    p.canvas->Create(p.w, p.h);
    p.panel->Resize(0, 0, p.w, p.h);
    p.manager->Resize(Sexy::Rect(0, 0, p.w, p.h), Sexy::Rect(0, 0, p.w, p.h));
  }
  p.manager->UpdateFrame();
  Sexy::Graphics g(p.canvas.get());
  p.manager->DrawWidgetsTo(&g);
  SDL_Surface *dest = SDL_GetWindowSurface(p.window);
  SDL_Surface *src = SDL_CreateRGBSurfaceWithFormatFrom(
      p.canvas->GetBits(), p.w, p.h, 32, p.w * 4, SDL_PIXELFORMAT_ARGB8888);
  if (dest && src) {
    SDL_BlitScaled(src, nullptr, dest, nullptr);
    SDL_UpdateWindowSurface(p.window);
  }
  if (src)
    SDL_FreeSurface(src);
  SDL_GL_MakeCurrent(static_cast<SDL_Window *>(p.app->mWindow),
                     p.app->mContext);
}
bool Runtime::ToolFocused() const {
  return impl->window && impl->visible &&
         (SDL_GetWindowFlags(impl->window) & SDL_WINDOW_INPUT_FOCUS);
}
bool Runtime::Event(const SDL_Event &e) {
  auto &p = *impl;
  if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F10 && !e.key.repeat) {
    Toggle();
    return true;
  }
  std::uint32_t id = 0;
  switch (e.type) {
  case SDL_WINDOWEVENT:
    id = e.window.windowID;
    break;
  case SDL_MOUSEMOTION:
    id = e.motion.windowID;
    break;
  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP:
    id = e.button.windowID;
    break;
  case SDL_MOUSEWHEEL:
    id = e.wheel.windowID;
    break;
  case SDL_KEYDOWN:
  case SDL_KEYUP:
    id = e.key.windowID;
    break;
  case SDL_TEXTINPUT:
    id = e.text.windowID;
    break;
  case SDL_TEXTEDITING:
    id = e.edit.windowID;
    break;
  }
  if (p.preview->mVisible &&
      id == SDL_GetWindowID(static_cast<SDL_Window *>(p.app->mWindow))) {
    if (!p.service.state.board || p.service.state.replay) {
      p.preview->SetVisible(false);
      return false;
    }
    if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN ||
        e.type == SDL_MOUSEBUTTONUP) {
      int x = e.type == SDL_MOUSEMOTION ? e.motion.x : e.button.x,
          y = e.type == SDL_MOUSEMOTION ? e.motion.y : e.button.y;
      p.app->mWidgetManager->RemapMouse(x, y);
      if (e.type == SDL_MOUSEBUTTONDOWN &&
          ((y < 48 && x > p.app->mWidth - 100) ||
           e.button.button == SDL_BUTTON_RIGHT))
        p.preview->SetVisible(false);
      else if (y > 48) {
        auto *b = p.app->mBoard;
        int col = b->PixelToGridX(x, y), row = b->PixelToGridY(x, y);
        p.preview->col = col;
        p.preview->row = row;
        if (e.type == SDL_MOUSEBUTTONDOWN && row >= 0 &&
            row < p.service.state.rows && col >= 0 && col < 9) {
          auto c = p.preview->command;
          c.row = row;
          c.col = col;
          p.service.Submit(std::move(c));
        }
      }
      p.preview->MarkDirty();
      return true;
    }
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
      p.preview->SetVisible(false);
      return true;
    }
  }
  if (!p.window || id != SDL_GetWindowID(p.window))
    return false;
  auto &manager = *p.manager;
  switch (e.type) {
  case SDL_WINDOWEVENT:
    if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
      if (p.visible)
        Toggle();
    } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
      manager.GotFocus();
      p.app->SetTrainerFocus(true);
    } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
      manager.LostFocus();
      p.app->SetTrainerFocus(Enabled(Flag::Background) ||
                             SDL_GetKeyboardFocus() ==
                                 static_cast<SDL_Window *>(p.app->mWindow));
    }
    break;
  case SDL_MOUSEMOTION:
    manager.MouseMove(e.motion.x, e.motion.y);
    break;
  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP: {
    int b = e.button.button == SDL_BUTTON_LEFT    ? 1
            : e.button.button == SDL_BUTTON_RIGHT ? -1
                                                  : 3;
    if (e.type == SDL_MOUSEBUTTONDOWN)
      manager.MouseDown(e.button.x, e.button.y, b);
    else
      manager.MouseUp(e.button.x, e.button.y, b);
    break;
  }
  case SDL_MOUSEWHEEL:
    p.panel->MouseWheel(e.wheel.y);
    break;
  case SDL_KEYDOWN:
    if (e.key.keysym.sym == SDLK_ESCAPE) {
      Toggle();
      break;
    }
    manager.KeyDown(Key(e.key.keysym.sym));
    if (e.key.keysym.mod & (KMOD_CTRL | KMOD_GUI)) {
      SDL_Keycode key = e.key.keysym.sym;
      if (key == SDLK_a || key == SDLK_c || key == SDLK_v || key == SDLK_x ||
          key == SDLK_z)
        manager.KeyChar(char(key - SDLK_a + 1));
    }
    break;
  case SDL_KEYUP:
    manager.KeyUp(Key(e.key.keysym.sym));
    break;
  case SDL_TEXTINPUT:
    manager.KeyText(e.text.text);
    break;
  }
  return true;
}
bool HandleEvent(Sexy::SexyAppBase *app, const SDL_Event &e) {
  return active && owner == app && active->Event(e);
}
void Pump(Sexy::SexyAppBase *app) {
  if (active && owner == app)
    active->Pump();
}
bool HasToolFocus(Sexy::SexyAppBase *app) {
  return active && owner == app && active->ToolFocused();
}
void BeforePresent(Sexy::SexyAppBase *app) {
  if (!active || owner != app)
    return;
  auto &service = active->GetService();
  if (!service.screenshotRequested)
    return;
  service.screenshotRequested = false;
  int w = 0, h = 0;
  SDL_GL_GetDrawableSize(static_cast<SDL_Window *>(app->mWindow), &w, &h);
  if (w <= 0 || h <= 0 || w > 16384 || h > 16384)
    return;
  std::vector<unsigned char> rgba(static_cast<size_t>(w) * h * 4);
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
  ImageLib::Image image;
  image.mWidth = w;
  image.mHeight = h;
  image.mBits = std::make_unique<uint32_t[]>(static_cast<size_t>(w) * h);
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      auto *p = &rgba[(static_cast<size_t>(h - 1 - y) * w + x) * 4];
      image.mBits[static_cast<size_t>(y) * w + x] =
          0xff000000U | (uint32_t(p[0]) << 16) | (uint32_t(p[1]) << 8) | p[2];
    }
  auto dir = std::filesystem::path(Sexy::GetAppDataPath("trainer/screenshots"));
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  auto path =
      dir / ("pvz-" + std::to_string(SDL_GetPerformanceCounter()) + ".png");
  if (!ec && ImageLib::WritePNGImage(path.string(), &image)) {
    service.text = Sexy::PathToU8(path);
    service.messageEn = "Screenshot: " + path.string();
    service.messageZh = "截图：" + path.string();
  } else {
    service.messageEn = "Could not save screenshot";
    service.messageZh = "无法保存截图";
  }
}
} // namespace Trainer
#endif
