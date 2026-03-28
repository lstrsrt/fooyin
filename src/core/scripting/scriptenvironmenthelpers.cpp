#include <core/scripting/scriptenvironmenthelpers.h>

#include "library/librarymanager.h"

#include <QDir>

namespace Fooyin {
LibraryScriptEnvironment::LibraryScriptEnvironment(const LibraryManager* libraryManager)
    : m_libraryManager{libraryManager}
    , m_trackListContextPolicy{TrackListContextPolicy::Unresolved}
    , m_escapeRichText{false}
    , m_useVariousArtists{false}
    , m_fullStarSymbol{defaultRatingFullStarSymbol()}
    , m_halfStarSymbol{defaultRatingHalfStarSymbol()}
    , m_emptyStarSymbol{defaultRatingEmptyStarSymbol()}
{ }

void LibraryScriptEnvironment::setEvaluationPolicy(TrackListContextPolicy policy, QString placeholder,
                                                   bool escapeRichText, bool useVariousArtists)
{
    m_trackListContextPolicy = policy;
    m_trackListPlaceholder   = std::move(placeholder);
    m_escapeRichText         = escapeRichText;
    m_useVariousArtists      = useVariousArtists;
}

void LibraryScriptEnvironment::setRatingStarSymbols(const RatingStarSymbols& ratingSymbols)
{
    m_fullStarSymbol  = ratingSymbols.fullStarSymbol;
    m_halfStarSymbol  = ratingSymbols.halfStarSymbol;
    m_emptyStarSymbol = ratingSymbols.emptyStarSymbol;
}

const ScriptLibraryEnvironment* LibraryScriptEnvironment::libraryEnvironment() const
{
    return this;
}

const ScriptEvaluationEnvironment* LibraryScriptEnvironment::evaluationEnvironment() const
{
    return this;
}

QString LibraryScriptEnvironment::libraryName(const Track& track) const
{
    if(!m_libraryManager) {
        return {};
    }

    if(const auto library = m_libraryManager->libraryInfo(track.libraryId())) {
        return library->name;
    }

    return {};
}

QString LibraryScriptEnvironment::libraryPath(const Track& track) const
{
    if(!m_libraryManager) {
        return {};
    }

    if(const auto library = m_libraryManager->libraryInfo(track.libraryId())) {
        return library->path;
    }

    return {};
}

QString LibraryScriptEnvironment::relativePath(const Track& track) const
{
    const QString path = libraryPath(track);
    return path.isEmpty() ? QString{} : QDir{path}.relativeFilePath(track.prettyFilepath());
}

TrackListContextPolicy LibraryScriptEnvironment::trackListContextPolicy() const
{
    return m_trackListContextPolicy;
}

QString LibraryScriptEnvironment::trackListPlaceholder() const
{
    return m_trackListPlaceholder;
}

bool LibraryScriptEnvironment::escapeRichText() const
{
    return m_escapeRichText;
}

bool LibraryScriptEnvironment::useVariousArtists() const
{
    return m_useVariousArtists;
}

QString LibraryScriptEnvironment::ratingFullStarSymbol() const
{
    return m_fullStarSymbol.isEmpty() ? ScriptEvaluationEnvironment::ratingFullStarSymbol() : m_fullStarSymbol;
}

QString LibraryScriptEnvironment::ratingHalfStarSymbol() const
{
    return m_halfStarSymbol.isEmpty() ? ScriptEvaluationEnvironment::ratingHalfStarSymbol() : m_halfStarSymbol;
}

QString LibraryScriptEnvironment::ratingEmptyStarSymbol() const
{
    return m_emptyStarSymbol;
}
} // namespace Fooyin
