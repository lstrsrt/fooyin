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

#include "tagfilloperation.h"

#include "tagfillpattern.h"

#include <core/scripting/scriptparser.h>
#include <core/scripting/scripttrackwriter.h>
#include <utils/stringutils.h>

namespace Fooyin::TagEditor {
namespace {
QString sourceStringForTrack(FillSourceMode mode, const QString& sourceScript, const Track& track)
{
    switch(mode) {
        case FillSourceMode::Filename:
            return track.filename();
        case FillSourceMode::Other: {
            ScriptParser parser;
            return parser.evaluate(sourceScript, track);
        }
    }

    return {};
}

QString transformFillValue(const QString& value, bool autoCapitalise)
{
    return autoCapitalise ? Utils::capitalise(value) : value;
}
} // namespace

FillValuesResult calculateFillValues(const TrackList& tracks, const FillValuesOptions& options)
{
    FillValuesResult result;

    const FillPattern pattern = FillPattern::parse(options.pattern.trimmed());
    result.patternValid       = pattern.isValid();
    result.fields             = pattern.captureFields();
    result.previewRows.reserve(tracks.size());

    for(const Track& track : tracks) {
        const QString source = sourceStringForTrack(options.sourceMode, options.sourceScript, track);

        FillPreviewRow previewRow;
        previewRow.source = source;
        previewRow.values.resize(result.fields.size());

        if(result.patternValid) {
            if(const auto match = pattern.match(source); match.has_value()) {
                previewRow.matched = true;
                ++result.matchedTracks;

                Track updatedTrack{track};

                for(size_t i{0}; i < match->values.size(); ++i) {
                    const auto& [field, value]     = match->values.at(i);
                    const QString transformedValue = transformFillValue(value, options.autoCapitalise);
                    previewRow.values.at(i)        = transformedValue;

                    if(transformedValue.isEmpty()) {
                        continue;
                    }

                    setTrackScriptValue(field, fillFieldValue(field, transformedValue), updatedTrack);
                }

                if(!updatedTrack.sameDataAs(track)) {
                    result.tracks.emplace_back(std::move(updatedTrack));
                    ++result.changedTracks;
                }
            }
            else {
                ++result.unmatchedTracks;
            }
        }

        result.previewRows.emplace_back(std::move(previewRow));
    }

    return result;
}
} // namespace Fooyin::TagEditor
