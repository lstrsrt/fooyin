/*
 * Fooyin
 * Copyright © 2024, Luke Taylor <LukeT1@proton.me>
 *
 * Fooyin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Fooyin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Fooyin.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

namespace Fooyin::VgmInput {
constexpr auto RomPathSetting = "VgmInput/RomPath";

constexpr auto DefaultLoopCount     = 2;
constexpr auto LoopCountSetting     = "VgmInput/LoopCount";
constexpr auto DefaultFadeLength    = 4000;
constexpr auto FadeLengthSetting    = "VgmInput/FadeLength";
constexpr auto DefaultSilenceLength = 1000;
constexpr auto SilenceLengthSetting = "VgmInput/SilenceLength";

constexpr auto DefaultGuessTrack = true;
constexpr auto GuessTrackSetting = "VgmInput/GuessTrack";
} // namespace Fooyin::VgmInput
