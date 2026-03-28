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

    void setPlaylistData(const Playlist* playlist, const PlaylistTrackIndexes* playlistQueue,
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
    const PlaylistTrackIndexes* m_playlistQueue;
    const TrackList* m_tracks;
    std::vector<int> m_directQueueIndexes;
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

struct PlaybackScriptContextData
{
    PlaylistTrackIndexes playlistQueue;
    TrackList tracks;
    PlaylistScriptEnvironment environment;
    ScriptContext context;

    PlaybackScriptContextData();
    PlaybackScriptContextData(const PlaybackScriptContextData&)            = delete;
    PlaybackScriptContextData& operator=(const PlaybackScriptContextData&) = delete;
    PlaybackScriptContextData(PlaybackScriptContextData&& other) noexcept;
    PlaybackScriptContextData& operator=(PlaybackScriptContextData&& other) noexcept;
};

[[nodiscard]] const ScriptVariableProvider& artworkMarkerVariableProvider();
[[nodiscard]] const ScriptVariableProvider& playlistVariableProvider();
[[nodiscard]] PlaybackScriptContextData makePlaybackScriptContext(PlayerController* playerController,
                                                                  Playlist* playlist, TrackListContextPolicy policy,
                                                                  QString placeholder = {}, bool escapeRichText = false,
                                                                  bool useVariousArtists                 = false,
                                                                  const RatingStarSymbols& ratingSymbols = {});
} // namespace Fooyin
