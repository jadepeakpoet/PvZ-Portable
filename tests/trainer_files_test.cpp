// SPDX-License-Identifier: LGPL-3.0-or-later
// Build TrainerFiles.cpp with PVZ_TRAINER_FILES_TEST; native picker stubs below
// let these tests run on any desktop without opening OS dialogs.
#include "Trainer/TrainerFiles.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace {
std::filesystem::path outputRoot;
std::uint64_t pickerTicket = 0;
int pickerKind = -1;
std::string sharedPath;
void Check(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
std::string Utf8(const std::filesystem::path &path) {
  auto value = path.u8string();
  return {reinterpret_cast<const char *>(value.data()), value.size()};
}
} // namespace
namespace Sexy {
std::string GetAppDataPath(std::string_view relative) {
  return Utf8(outputRoot / std::filesystem::path(relative));
}
} // namespace Sexy
namespace Trainer::FilesDetail {
bool Available() { return true; }
void Choose(std::uint64_t ticket, int kind) {
  pickerTicket = ticket;
  pickerKind = kind;
}
bool Share(const std::string &path) {
  sharedPath = path;
  return true;
}
} // namespace Trainer::FilesDetail

int main(int argc, char **argv) {
  using namespace Trainer;
  try {
    outputRoot = argc > 1 ? std::filesystem::path(argv[1])
                          : std::filesystem::temp_directory_path() /
                                ("pvz-trainer-files-" +
                                 std::to_string(std::chrono::steady_clock::now()
                                                    .time_since_epoch()
                                                    .count()));
    std::filesystem::create_directories(outputRoot);
    int calls = 0;
    std::string result;
    auto gameThread = std::this_thread::get_id();
    ChooseFile(false, [&](std::string path) {
      Check(std::this_thread::get_id() == gameThread,
            "callback ran outside game thread");
      ++calls;
      result = std::move(path);
    });
    Check(pickerKind == 0, "open-file request did not reach platform");
    auto oldTicket = pickerTicket;
    std::thread native(
        [oldTicket] { FilesDetail::Complete(oldTicket, "/tmp/中文.txt"); });
    native.join();
    Check(calls == 0, "native completion ran callback before pump");
    PumpFileDialogs();
    Check(calls == 1 && result == "/tmp/中文.txt",
          "UTF8 callback result changed");

    ChooseDirectory([&](std::string) { ++calls; });
    Check(pickerKind == 2,
          "directory request was not accepted after completion");
    oldTicket = pickerTicket;
    int rejected = 0;
    ChooseFile(false, [&](std::string path) {
      Check(path.empty(), "busy request should be rejected with empty path");
      ++rejected;
    });
    Check(rejected == 0, "busy rejection must also be deferred");
    PumpFileDialogs();
    Check(rejected == 1, "busy request rejection was not delivered");
    CancelFileDialogs();
    FilesDetail::Complete(oldTicket, "stale");
    PumpFileDialogs();
    Check(calls == 1, "cancelled UI callback was invoked");

    ChooseFile(true, [&](std::string path) {
      ++calls;
      result = std::move(path);
    });
    Check(pickerKind == 1, "save request did not reach platform");
    FilesDetail::Complete(oldTicket, "old");
    PumpFileDialogs();
    Check(calls == 1, "old picker ticket completed a new request");
    FilesDetail::Complete(pickerTicket, "");
    PumpFileDialogs();
    Check(calls == 2 && result.empty(),
          "picker cancellation was not delivered");

    auto source = outputRoot / "source";
    auto unicode = source / std::filesystem::path(u8"目录");
    std::filesystem::create_directories(unicode / "empty");
    std::ofstream(unicode / std::filesystem::path(u8"中文.txt"),
                  std::ios::binary)
        << "Chinese UTF8 payload";
    std::ofstream(source / "plain.bin", std::ios::binary).write("A\0B", 3);
    Check(ShareFile(Utf8(source)), "directory share was rejected");
    Check(!sharedPath.empty() &&
              std::filesystem::is_regular_file(
                  std::filesystem::path(std::u8string(
                      reinterpret_cast<const char8_t *>(sharedPath.data()),
                      sharedPath.size()))),
          "directory was not converted to a ZIP file");
    std::ofstream(outputRoot / "zip-path.txt", std::ios::binary) << sharedPath;
    Check(FilesDetail::PrepareSharePath(Utf8(source / "plain.bin")) ==
              Utf8(source / "plain.bin"),
          "regular file share should not repackage the file");
    Check(!ShareFile(Utf8(outputRoot / "missing")),
          "missing source reported success");

    // Windows machines may not grant the symlink privilege. Verify this
    // guard whenever the OS permits creating one, and report a skip otherwise.
    std::error_code ec;
    std::filesystem::create_directory_symlink(outputRoot, source / "link", ec);
    if (!ec) {
      bool rejectedLink = false;
      try {
        FilesDetail::PrepareSharePath(Utf8(source));
      } catch (const std::exception &) {
        rejectedLink = true;
      }
      Check(rejectedLink, "symlink export was accepted");
      std::filesystem::remove(source / "link");
    } else {
      std::cout << "SKIP symlink creation: " << ec.message() << '\n';
    }
    CancelFileDialogs();
    std::cout << "PASS deferred callbacks, UTF8, busy/cancel/stale tickets, "
                 "ZIP staging\n";
    std::cout << "ZIP_MANIFEST=" << Utf8(outputRoot / "zip-path.txt") << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
