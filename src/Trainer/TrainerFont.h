// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "graphics/Font.h"
#include <memory>
#include <string>

namespace Trainer {
// Uses a trusted local TrueType/OpenType font, independently of the game
// assets. Returns nullptr if no suitable font is installed. UI must handle that
// case.
std::unique_ptr<Sexy::_Font> CreateFont(int pixelHeight);
bool HasChineseFont();
const std::string &FontSource();
} // namespace Trainer
