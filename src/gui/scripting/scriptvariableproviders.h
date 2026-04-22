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

#include <core/player/playbackqueue.h>
#include <core/ratingsymbols.h>
#include <core/scripting/scriptproviders.h>

namespace Fooyin {
class PlayerController;

constexpr auto PlayingIcon = "%playingicon%";

class PlaylistScriptEnvironment : public ScriptEnvironment,
                                  public ScriptPlaylistEnvironment,
                                  public ScriptTrackListEnvironment,
                                  public ScriptPlaybackEnvironment,
                                  public ScriptEvaluationEnvironment
{
public:
    PlaylistScriptEnvironment();

    void setPlaylistData(const Playlist* playlist, const PlaybackQueue* playbackQueue,
                         const TrackList* tracks = nullptr, int queueTotal = 0);
    void setQueueState(std::span<const int> queueIndexes, int queueTotal);
    void setTrackState(int playlistTrackIndex, int currentPlayingTrackIndex, int currentPlayingTrackId, int trackDepth);
    void setPlaybackState(uint64_t currentPosition, uint64_t currentTrackDuration, int bitrate,
                          Player::PlayState playState);
    void setEvaluationPolicy(TrackListContextPolicy policy, QString placeholder, bool escapeRichText,
                             bool useVariousArtists = false);

    void setRatingStarSymbols(const RatingStarSymbols& ratingSymbols);

    [[nodiscard]] const ScriptPlaylistEnvironment* playlistEnvironment() const override;
    [[nodiscard]] const ScriptTrackListEnvironment* trackListEnvironment() const override;
    [[nodiscard]] const ScriptPlaybackEnvironment* playbackEnvironment() const override;
    [[nodiscard]] const ScriptEvaluationEnvironment* evaluationEnvironment() const override;

    [[nodiscard]] int currentPlaylistTrackIndex() const override;
    [[nodiscard]] int currentPlayingTrackIndex() const override;
    [[nodiscard]] int currentPlayingTrackId() const override;
    [[nodiscard]] int playlistTrackCount() const override;
    [[nodiscard]] int trackDepth() const override;
    [[nodiscard]] std::span<const int> currentQueueIndexes() const override;
    [[nodiscard]] int currentQueueTotal() const override;
    [[nodiscard]] const TrackList* trackList() const override;
    [[nodiscard]] uint64_t currentPosition() const override;
    [[nodiscard]] uint64_t currentTrackDuration() const override;
    [[nodiscard]] int bitrate() const override;
    [[nodiscard]] Player::PlayState playState() const override;
    [[nodiscard]] TrackListContextPolicy trackListContextPolicy() const override;
    [[nodiscard]] QString trackListPlaceholder() const override;
    [[nodiscard]] bool escapeRichText() const override;
    [[nodiscard]] bool useVariousArtists() const override;
    [[nodiscard]] QString ratingFullStarSymbol() const override;
    [[nodiscard]] QString ratingHalfStarSymbol() const override;
    [[nodiscard]] QString ratingEmptyStarSymbol() const override;
    [[nodiscard]] bool hasDirectQueueState() const;

private:
    const Playlist* m_playlist;
    const PlaybackQueue* m_playbackQueue;
    const TrackList* m_tracks;
    std::vector<int> m_directQueueIndexes;
    mutable std::vector<int> m_currentQueueIndexes;
    int m_playlistTrackIndex;
    int m_currentPlayingTrackIndex;
    int m_currentPlayingTrackId;
    int m_trackDepth;
    int m_queueTotal;
    uint64_t m_currentPosition;
    uint64_t m_currentTrackDuration;
    int m_bitrate;
    Player::PlayState m_playState;
    TrackListContextPolicy m_trackListContextPolicy;
    QString m_trackListPlaceholder;
    bool m_escapeRichText;
    bool m_useVariousArtists;
    RatingStarSymbols m_ratingSymbols;
    bool m_hasDirectQueueState;
};

struct PlaybackScriptContext
{
    PlaylistScriptEnvironment environment;
    ScriptContext context;

    PlaybackScriptContext();
    PlaybackScriptContext(const PlaybackScriptContext&)            = delete;
    PlaybackScriptContext& operator=(const PlaybackScriptContext&) = delete;
    PlaybackScriptContext(PlaybackScriptContext&& other) noexcept;
    PlaybackScriptContext& operator=(PlaybackScriptContext&& other) noexcept;
};

[[nodiscard]] const ScriptVariableProvider& artworkMarkerVariableProvider();
[[nodiscard]] const ScriptVariableProvider& playlistVariableProvider();
[[nodiscard]] PlaybackScriptContext makePlaybackScriptContext(PlayerController* playerController, Playlist* playlist,
                                                              TrackListContextPolicy policy, QString placeholder = {},
                                                              bool escapeRichText                    = false,
                                                              bool useVariousArtists                 = false,
                                                              const RatingStarSymbols& ratingSymbols = {});
} // namespace Fooyin
