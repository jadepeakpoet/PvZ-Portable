// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once
#include <cstdint>
#include <functional>
#include <string>
namespace Trainer {
// Completion always runs from PumpFileDialogs on the game thread. Empty path
// means cancellation/error. Mobile imports are copied into app-owned storage.
void ChooseFile(bool save, std::function<void(std::string)> done);
void ChooseDirectory(std::function<void(std::string)> done);
void PumpFileDialogs();
void CancelFileDialogs();
bool CanChooseFiles();
// Mobile save requests supply an app-local staging filename. Once written,
// export it through the platform's document/share UI using this function.
// Directories become a bounded stored ZIP in trainer/exports before sharing.
bool ShareFile(const std::string &path);
// Desktop reveals the directory; mobile exports a ZIP through the document UI.
bool RevealFolder(const std::string &path);
namespace FilesDetail {
void Complete(std::uint64_t ticket, std::string path);
std::string PrepareSharePath(const std::string &path);
#if defined(__APPLE__) || defined(PVZ_TRAINER_FILES_TEST)
bool Available();
void Choose(std::uint64_t ticket, int kind);
bool Share(const std::string &path);
#endif
} // namespace FilesDetail
} // namespace Trainer
