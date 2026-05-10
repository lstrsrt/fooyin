/*
 * Fooyin
 * Copyright © 2026, Luke Taylor <luket@pm.me>
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

#include <core/track.h>

#include <QString>
#include <QStringList>

#include <cstdint>
#include <vector>

namespace Fooyin::TagEditor {
enum class FillSourceMode : uint8_t
{
    Filename = 0,
    Other,
};

struct FillValuesOptions
{
    FillSourceMode sourceMode{FillSourceMode::Filename};
    QString sourceScript;
    QString pattern;
    QStringList multiValueSeparators;
    bool autoCapitalise{false};
};

struct FillPreviewRow
{
    QString source;
    std::vector<QString> values;
    bool matched{false};
};

struct FillValuesResult
{
    std::vector<QString> fields;
    std::vector<FillPreviewRow> previewRows;
    TrackList tracks;
    int matchedTracks{0};
    int changedTracks{0};
    int unmatchedTracks{0};
    bool patternValid{false};
};

[[nodiscard]] FillValuesResult calculateFillValues(const TrackList& tracks, const FillValuesOptions& options);
} // namespace Fooyin::TagEditor
