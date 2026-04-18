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

#include "fycore_export.h"

#include <core/ratingsymbols.h>
#include <core/scripting/scripttypes.h>

namespace Fooyin {
class LibraryManager;

/*!
 * Reusable environment adapter for library-backed scripting surfaces.
 *
 * This helper exposes library lookups plus a small evaluation-policy surface. It is
 * intended for callers that need basic library variables without implementing their
 * own `ScriptEnvironment`.
 */
class FYCORE_EXPORT LibraryScriptEnvironment : public ScriptEnvironment,
                                               public ScriptLibraryEnvironment,
                                               public ScriptEvaluationEnvironment
{
public:
    explicit LibraryScriptEnvironment(const LibraryManager* libraryManager);

    void setEvaluationPolicy(TrackListContextPolicy policy, QString placeholder = {}, bool escapeRichText = false,
                             bool useVariousArtists = false);
    void setRatingStarSymbols(const RatingStarSymbols& ratingSymbols);

    [[nodiscard]] const ScriptLibraryEnvironment* libraryEnvironment() const override;
    [[nodiscard]] const ScriptEvaluationEnvironment* evaluationEnvironment() const override;

    [[nodiscard]] QString libraryName(const Track& track) const override;
    [[nodiscard]] QString libraryPath(const Track& track) const override;
    [[nodiscard]] QString relativePath(const Track& track) const override;
    [[nodiscard]] TrackListContextPolicy trackListContextPolicy() const override;
    [[nodiscard]] QString trackListPlaceholder() const override;
    [[nodiscard]] bool escapeRichText() const override;
    [[nodiscard]] bool useVariousArtists() const override;
    [[nodiscard]] QString ratingFullStarSymbol() const override;
    [[nodiscard]] QString ratingHalfStarSymbol() const override;
    [[nodiscard]] QString ratingEmptyStarSymbol() const override;

private:
    const LibraryManager* m_libraryManager;
    TrackListContextPolicy m_trackListContextPolicy;
    QString m_trackListPlaceholder;
    bool m_escapeRichText;
    bool m_useVariousArtists;
    QString m_fullStarSymbol;
    QString m_halfStarSymbol;
    QString m_emptyStarSymbol;
};
} // namespace Fooyin
