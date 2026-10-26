// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef PVZ_TRAINER
#include "TrainerFiles.h"
#include "Common.h"
#include <SDL.h>
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>
#include <zlib.h>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#elif defined(__ANDROID__) && !defined(__TERMUX__)
#include <jni.h>
#elif !defined(__APPLE__) && !defined(__EMSCRIPTEN__) && !defined(__SWITCH__)
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

namespace Trainer {
namespace {
struct State {
  std::mutex mutex;
  std::uint64_t ticket = 0;
  std::function<void(std::string)> callback;
  std::optional<std::string> result;
  std::vector<std::function<void()>> deferred;
};
// Native dialogs can outlive the game UI. Process-lifetime state lets their
// completion safely discard cancelled tickets without touching destroyed UI.
State &DialogState() {
  static auto *state = new State;
  return *state;
}

#if defined(PVZ_TRAINER_FILES_TEST)
void PlatformChoose(std::uint64_t ticket, int kind) {
  FilesDetail::Choose(ticket, kind);
}
#elif defined(_WIN32)
std::string UTF8(const std::wstring &value) {
  if (value.empty())
    return {};
  int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), int(value.size()),
                                 nullptr, 0, nullptr, nullptr);
  std::string out(size, '\0');
  WideCharToMultiByte(CP_UTF8, 0, value.data(), int(value.size()), out.data(),
                      size, nullptr, nullptr);
  return out;
}
void PlatformChoose(std::uint64_t ticket, int kind) {
  std::thread([ticket, kind] {
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    std::wstring path;
    if (kind == 2) {
      IFileOpenDialog *dialog = nullptr;
      if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                                     CLSCTX_INPROC_SERVER,
                                     IID_PPV_ARGS(&dialog)))) {
        DWORD flags = 0;
        dialog->GetOptions(&flags);
        dialog->SetOptions(flags | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM |
                           FOS_PATHMUSTEXIST);
        if (SUCCEEDED(dialog->Show(nullptr))) {
          IShellItem *item = nullptr;
          if (SUCCEEDED(dialog->GetResult(&item))) {
            PWSTR value = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &value))) {
              path = value;
              CoTaskMemFree(value);
            }
            item->Release();
          }
        }
        dialog->Release();
      }
    } else {
      wchar_t filename[32768] = L"";
      OPENFILENAMEW dialog{};
      dialog.lStructSize = sizeof(dialog);
      dialog.lpstrFile = filename;
      dialog.nMaxFile = 32768;
      dialog.lpstrFilter = L"All files\0*.*\0";
      dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
                     (kind == 1 ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
      if (kind == 1 ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog))
        path = filename;
    }
    if (SUCCEEDED(init))
      CoUninitialize();
    FilesDetail::Complete(ticket, UTF8(path));
  }).detach();
}
#elif defined(__ANDROID__) && !defined(__TERMUX__)
jstring JavaString(JNIEnv *env, const std::string &text) {
  std::vector<jchar> utf16;
  size_t pos = 0;
  char32_t cp = 0;
  while (Sexy::UTF8DecodeNext(text, pos, cp)) {
    if (cp <= 0xffff)
      utf16.push_back(jchar(cp));
    else if (cp <= 0x10ffff) {
      cp -= 0x10000;
      utf16.push_back(jchar(0xd800 + (cp >> 10)));
      utf16.push_back(jchar(0xdc00 + (cp & 1023)));
    }
  }
  return env->NewString(utf16.data(), jsize(utf16.size()));
}
std::string NativeString(JNIEnv *env, jstring text) {
  const jchar *chars = env->GetStringChars(text, nullptr);
  if (!chars)
    return {};
  jsize length = env->GetStringLength(text);
  std::string out;
  for (jsize i = 0; i < length; ++i) {
    char32_t cp = chars[i];
    if (cp >= 0xd800 && cp <= 0xdbff && i + 1 < length &&
        chars[i + 1] >= 0xdc00 && chars[i + 1] <= 0xdfff)
      cp = 0x10000 + ((cp - 0xd800) << 10) + (chars[++i] - 0xdc00);
    if (cp < 0x80)
      out += char(cp);
    else if (cp < 0x800) {
      out += char(0xc0 | (cp >> 6));
      out += char(0x80 | (cp & 63));
    } else if (cp < 0x10000) {
      out += char(0xe0 | (cp >> 12));
      out += char(0x80 | ((cp >> 6) & 63));
      out += char(0x80 | (cp & 63));
    } else {
      out += char(0xf0 | (cp >> 18));
      out += char(0x80 | ((cp >> 12) & 63));
      out += char(0x80 | ((cp >> 6) & 63));
      out += char(0x80 | (cp & 63));
    }
  }
  env->ReleaseStringChars(text, chars);
  return out;
}
bool AndroidCall(const char *method, const char *signature, int kind = 0,
                 std::uint64_t ticket = 0, const std::string *path = nullptr) {
  auto *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
  jobject activity = SDL_AndroidGetActivity();
  if (!env || !activity)
    return false;
  jclass cls = env->GetObjectClass(activity);
  jmethodID id = env->GetMethodID(cls, method, signature);
  bool ok = id && !env->ExceptionCheck();
  if (ok) {
    if (path) {
      jstring text = JavaString(env, *path);
      env->CallVoidMethod(activity, id, text);
      env->DeleteLocalRef(text);
    } else
      env->CallVoidMethod(activity, id, jint(kind), jlong(ticket));
    ok = !env->ExceptionCheck();
  }
  if (env->ExceptionCheck())
    env->ExceptionClear();
  env->DeleteLocalRef(cls);
  env->DeleteLocalRef(activity);
  return ok;
}
void PlatformChoose(std::uint64_t ticket, int kind) {
  if (!AndroidCall("trainerChoose", "(IJ)V", kind, ticket))
    FilesDetail::Complete(ticket, {});
}
void PollAndroid() {
  auto *env = static_cast<JNIEnv *>(SDL_AndroidGetJNIEnv());
  jobject activity = SDL_AndroidGetActivity();
  if (!env || !activity)
    return;
  jclass cls = env->GetObjectClass(activity);
  auto id = env->GetMethodID(cls, "trainerPoll", "()Ljava/lang/String;");
  if (id && !env->ExceptionCheck()) {
    auto result = static_cast<jstring>(env->CallObjectMethod(activity, id));
    if (result) {
      std::string value = NativeString(env, result);
      auto split = value.find('\n');
      if (split != std::string::npos)
        try {
          FilesDetail::Complete(std::stoull(value.substr(0, split)),
                                value.substr(split + 1));
        } catch (...) {
        }
      env->DeleteLocalRef(result);
    }
  }
  if (env->ExceptionCheck())
    env->ExceptionClear();
  env->DeleteLocalRef(cls);
  env->DeleteLocalRef(activity);
}
#elif defined(__APPLE__)
void PlatformChoose(std::uint64_t ticket, int kind) {
  FilesDetail::Choose(ticket, kind);
}
#elif !defined(__EMSCRIPTEN__) && !defined(__SWITCH__)
const char *Zenity() {
  for (const char *path : {"/usr/bin/zenity", "/usr/local/bin/zenity"})
    if (access(path, X_OK) == 0)
      return path;
  return nullptr;
}
void PlatformChoose(std::uint64_t ticket, int kind) {
  const char *executable = Zenity();
  if (!executable) {
    FilesDetail::Complete(ticket, {});
    return;
  }
  std::thread([ticket, kind, executable] {
    int fds[2];
    if (pipe(fds) != 0) {
      FilesDetail::Complete(ticket, {});
      return;
    }
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, fds[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, fds[0]);
    posix_spawn_file_actions_addclose(&actions, fds[1]);
    char *args[] = {const_cast<char *>(executable),
                    const_cast<char *>("--file-selection"),
                    const_cast<char *>("--timeout=120"),
                    const_cast<char *>(kind == 2   ? "--directory"
                                       : kind == 1 ? "--save"
                                                   : "--title=PvZ Toolkit"),
                    nullptr};
    pid_t pid = 0;
    int error = posix_spawn(&pid, executable, &actions, nullptr, args, environ);
    posix_spawn_file_actions_destroy(&actions);
    close(fds[1]);
    std::string path;
    char buffer[1024];
    ssize_t count;
    if (!error)
      while ((count = read(fds[0], buffer, sizeof(buffer))) > 0) {
        if (path.size() + count > 32768) {
          path.clear();
          break;
        }
        path.append(buffer, count);
      }
    close(fds[0]);
    int status = 0;
    if (!error)
      while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
      }
    if (error || !WIFEXITED(status) || WEXITSTATUS(status))
      path.clear();
    while (!path.empty() && (path.back() == '\n' || path.back() == '\r'))
      path.pop_back();
    FilesDetail::Complete(ticket, std::move(path));
  }).detach();
}
#else
void PlatformChoose(std::uint64_t ticket, int) {
  FilesDetail::Complete(ticket, {});
}
#endif

void Request(int kind, std::function<void(std::string)> done) {
  std::uint64_t ticket = 0;
  {
    auto &state = DialogState();
    std::lock_guard lock(state.mutex);
    if (!state.callback) {
      ticket = ++state.ticket;
      state.callback = std::move(done);
      state.result.reset();
    } else if (done)
      state.deferred.push_back([done = std::move(done)] { done({}); });
  }
  if (!ticket)
    return;
  PlatformChoose(ticket, kind);
}
} // namespace
namespace FilesDetail {
void Complete(std::uint64_t ticket, std::string path) {
  auto &state = DialogState();
  std::lock_guard lock(state.mutex);
  if (ticket == state.ticket && state.callback)
    state.result = std::move(path);
}
std::string PrepareSharePath(const std::string &path) {
  auto source = Sexy::PathFromU8(path);
  if (std::filesystem::is_symlink(source))
    throw std::runtime_error("Cannot export symlink");
  if (std::filesystem::is_regular_file(source))
    return path;
  if (!std::filesystem::is_directory(source))
    throw std::runtime_error("Export source does not exist");
  constexpr std::uint64_t limit = 512ULL * 1024 * 1024;
  struct Entry {
    std::filesystem::path path;
    std::string name;
    bool directory;
    std::uint32_t crc = 0, size = 0, offset = 0;
  };
  std::vector<Entry> entries;
  std::uint64_t expected = 0;
  for (const auto &item :
       std::filesystem::recursive_directory_iterator(source)) {
    if (item.is_symlink())
      throw std::runtime_error("Cannot export symlink");
    if (!item.is_directory() && !item.is_regular_file())
      throw std::runtime_error("Unsupported export entry");
    if (entries.size() >= 10000)
      throw std::runtime_error("Export has too many entries");
    auto relative = item.path().lexically_relative(source);
    for (const auto &part : relative)
      if (part == ".." || part == ".")
        throw std::runtime_error("Unsafe export path");
    auto utf8 = relative.generic_u8string();
    std::string name(reinterpret_cast<const char *>(utf8.data()), utf8.size());
    bool directory = item.is_directory();
    if (directory)
      name += '/';
    if (name.empty() || name.size() > 65535 ||
        name.find(':') != std::string::npos ||
        name.find('\\') != std::string::npos)
      throw std::runtime_error("Invalid export path");
    if (!directory) {
      expected += item.file_size();
      if (expected > limit)
        throw std::runtime_error("Export exceeds 512 MB");
    }
    entries.push_back({item.path(), std::move(name), directory});
  }
  auto root = Sexy::PathFromU8(Sexy::GetAppDataPath("trainer/exports"));
  std::filesystem::create_directories(root);
  auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
  auto output = root / ("toolkit-export-" + std::to_string(stamp) + ".zip");
  if (std::filesystem::exists(output))
    throw std::runtime_error("Export destination exists");
  try {
    std::ofstream out(output, std::ios::binary);
    if (!out)
      throw std::runtime_error("Cannot create export ZIP");
    auto le = [&](std::uint32_t value, int width) {
      for (int i = 0; i < width; ++i)
        out.put(char(value >> (i * 8)));
    };
    std::uint64_t actual = 0;
    char buffer[65536];
    for (auto &entry : entries) {
      entry.offset = static_cast<std::uint32_t>(out.tellp());
      le(0x04034b50, 4);
      le(20, 2);
      le(0x808, 2);
      le(0, 2);
      le(0, 2);
      le(0x21, 2);
      le(0, 4);
      le(0, 4);
      le(0, 4);
      le(entry.name.size(), 2);
      le(0, 2);
      out.write(entry.name.data(), entry.name.size());
      uLong crc = crc32(0, nullptr, 0);
      if (!entry.directory) {
        std::ifstream in(entry.path, std::ios::binary);
        if (!in)
          throw std::runtime_error("Cannot read export entry");
        while (in) {
          in.read(buffer, sizeof(buffer));
          auto n = in.gcount();
          actual += n;
          if (actual > limit)
            throw std::runtime_error("Export exceeds 512 MB");
          out.write(buffer, n);
          crc = crc32(crc, reinterpret_cast<const Bytef *>(buffer),
                      static_cast<uInt>(n));
          entry.size += n;
        }
        if (!in.eof())
          throw std::runtime_error("Cannot read export entry");
      }
      entry.crc = static_cast<std::uint32_t>(crc);
      le(0x08074b50, 4);
      le(entry.crc, 4);
      le(entry.size, 4);
      le(entry.size, 4);
      if (!out)
        throw std::runtime_error("Cannot write export ZIP");
    }
    auto central = static_cast<std::uint32_t>(out.tellp());
    for (const auto &entry : entries) {
      le(0x02014b50, 4);
      le(20, 2);
      le(20, 2);
      le(0x808, 2);
      le(0, 2);
      le(0, 2);
      le(0x21, 2);
      le(entry.crc, 4);
      le(entry.size, 4);
      le(entry.size, 4);
      le(entry.name.size(), 2);
      le(0, 2);
      le(0, 2);
      le(0, 2);
      le(0, 2);
      le(entry.directory ? 0x10 : 0, 4);
      le(entry.offset, 4);
      out.write(entry.name.data(), entry.name.size());
    }
    auto end = static_cast<std::uint32_t>(out.tellp());
    le(0x06054b50, 4);
    le(0, 2);
    le(0, 2);
    le(entries.size(), 2);
    le(entries.size(), 2);
    le(end - central, 4);
    le(central, 4);
    le(0, 2);
    out.close();
    if (!out)
      throw std::runtime_error("Cannot finish export ZIP");
  } catch (...) {
    std::error_code ec;
    std::filesystem::remove(output, ec);
    throw;
  }
  auto utf8 = output.u8string();
  return {reinterpret_cast<const char *>(utf8.data()), utf8.size()};
}
} // namespace FilesDetail
void ChooseFile(bool save, std::function<void(std::string)> done) {
  Request(save ? 1 : 0, std::move(done));
}
void ChooseDirectory(std::function<void(std::string)> done) {
  Request(2, std::move(done));
}
void PumpFileDialogs() {
#if defined(__ANDROID__) && !defined(__TERMUX__)
  PollAndroid();
#endif
  std::function<void(std::string)> callback;
  std::string result;
  std::vector<std::function<void()>> deferred;
  {
    auto &state = DialogState();
    std::lock_guard lock(state.mutex);
    deferred.swap(state.deferred);
    if (state.result && state.callback) {
      callback = std::move(state.callback);
      state.callback = {};
      result = std::move(*state.result);
      state.result.reset();
    }
  }
  if (callback)
    callback(std::move(result));
  for (auto &completion : deferred)
    completion();
}
void CancelFileDialogs() {
  auto &state = DialogState();
  std::lock_guard lock(state.mutex);
  ++state.ticket;
  state.callback = {};
  state.result.reset();
  state.deferred.clear();
}
bool CanChooseFiles() {
#if defined(__APPLE__) || defined(PVZ_TRAINER_FILES_TEST)
  return FilesDetail::Available();
#elif defined(_WIN32) || (defined(__ANDROID__) && !defined(__TERMUX__))
  return true;
#elif !defined(__EMSCRIPTEN__) && !defined(__SWITCH__)
  return Zenity() != nullptr;
#else
  return false;
#endif
}
bool ShareFile(const std::string &path) {
  std::string prepared;
  try {
    prepared = FilesDetail::PrepareSharePath(path);
  } catch (...) {
    return false;
  }
#if defined(__APPLE__) || defined(PVZ_TRAINER_FILES_TEST)
  return FilesDetail::Share(prepared);
#elif defined(__ANDROID__) && !defined(__TERMUX__)
  return AndroidCall("trainerShare", "(Ljava/lang/String;)V", 0, 0, &prepared);
#else
  std::error_code ec;
  auto full = std::filesystem::absolute(Sexy::PathFromU8(prepared), ec);
  if (ec)
    return false;
  return RevealFolder(Sexy::PathToU8(full.parent_path()));
#endif
}
bool RevealFolder(const std::string &path) {
#if defined(__ANDROID__) || defined(__IPHONEOS__)
  return ShareFile(path);
#else
  std::error_code ec;
  auto full = std::filesystem::absolute(Sexy::PathFromU8(path), ec);
  if (ec || !std::filesystem::is_directory(full, ec))
    return false;
  auto bytes = full.generic_u8string();
  std::string url = "file://";
#ifdef _WIN32
  url += '/';
#endif
  static const char hex[] = "0123456789ABCDEF";
  for (unsigned char c : bytes) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '/' || c == ':' || c == '-' ||
        c == '_' || c == '.' || c == '~')
      url += char(c);
    else {
      url += '%';
      url += hex[c >> 4];
      url += hex[c & 15];
    }
  }
  return SDL_OpenURL(url.c_str()) == 0;
#endif
}
} // namespace Trainer
#endif
