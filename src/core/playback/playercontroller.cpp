/*
 * Fooyin
 * Copyright © 2022, Luke Taylor <LukeT1@proton.me>
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

#include "core/playlist/playlisthandler.h"
#include <core/player/playercontroller.h>

#include <core/player/playbackqueue.h>

#include <core/coresettings.h>
#include <core/track.h>
#include <utils/settings/settingsmanager.h>

#include <QLoggingCategory>

#include <optional>

namespace Fooyin {
Q_LOGGING_CATEGORY(PLAYER_CONTROLLER, "fy.playercontroller")

namespace {
bool sameTrackIdentity(const Track& lhs, const Track& rhs)
{
    if(!lhs.isValid() || !rhs.isValid()) {
        return false;
    }

    if(lhs.id() >= 0 && rhs.id() >= 0) {
        return lhs.id() == rhs.id();
    }

    return lhs.uniqueFilepath() == rhs.uniqueFilepath() && lhs.subsong() == rhs.subsong()
        && lhs.offset() == rhs.offset() && lhs.duration() == rhs.duration();
}
} // namespace

class PlayerControllerPrivate
{
public:
    struct UpcomingTrackResolution
    {
        Player::UpcomingTrack upcoming;
        Playlist* activePlaylist{nullptr};
        const char* source{"unknown"};
        int previewIndex{-1};
    };

    PlayerControllerPrivate(PlayerController* self, SettingsManager* settings, PlaylistHandler* playlistHandler)
        : m_self{self}
        , m_settings{settings}
        , m_playlistHandler{playlistHandler}
        , m_playMode{static_cast<Playlist::PlayModes>(m_settings->value<Settings::Core::PlayMode>())}
    { }

    [[nodiscard]] uint64_t nextPlaybackItemId()
    {
        uint64_t itemId = m_nextPlaybackItemId++;
        if(itemId == 0) {
            itemId = m_nextPlaybackItemId++;
        }
        if(m_nextPlaybackItemId == 0) {
            m_nextPlaybackItemId = 1;
        }
        return itemId;
    }

    void emitPositionSignals(uint64_t ms)
    {
        emit m_self->positionChanged(ms);

        const uint64_t seconds = ms / 1000;
        if(!m_hasLastPositionSecond || m_lastPositionSecond != seconds) {
            m_lastPositionSecond    = seconds;
            m_hasLastPositionSecond = true;
            emit m_self->positionChangedSeconds(seconds);
        }
    }

    bool updateBitrate(int bitrate)
    {
        if(std::exchange(m_bitrate, bitrate) != bitrate) {
            emit m_self->bitrateChanged(bitrate);
            return true;
        }

        return false;
    }

    void changeTrack(const PlaylistTrack& track, const Player::TrackChangeContext& context)
    {
        m_currentTrack         = track;
        m_totalDuration        = m_currentTrack.track.duration();
        m_position             = 0;
        m_timeListened         = 0;
        m_counted              = false;
        m_lastChangeContext    = context;
        m_pendingChangeContext = {};

        if(m_totalDuration > 200) {
            m_playedThreshold = static_cast<uint64_t>(static_cast<double>(m_totalDuration - 200)
                                                      * m_settings->value<Settings::Core::PlayedThreshold>());
        }

        m_scheduledTrack = {};
    }

    [[nodiscard]] Player::TrackChangeRequest withPlaybackItemId(Player::TrackChangeRequest request)
    {
        if(request.itemId == 0 && request.track.isValid()) {
            request.itemId = nextPlaybackItemId();
        }
        return request;
    }

    [[nodiscard]] UpcomingTrackResolution resolveUpcomingTrack() const
    {
        UpcomingTrackResolution resolution;
        resolution.activePlaylist = m_playlistHandler ? m_playlistHandler->activePlaylist() : nullptr;

        if(m_settings->value<Settings::Core::StopAfterCurrent>()) {
            resolution.source = "stop-after-current";
            return resolution;
        }

        if(m_scheduledTrack.isValid()) {
            resolution.upcoming = {
                .track        = m_scheduledTrack,
                .isQueueTrack = false,
            };
            resolution.source = "scheduled-track";
            return resolution;
        }

        if(!m_queue.empty()) {
            resolution.upcoming = {
                .track        = m_queue.nextTrack(),
                .isQueueTrack = true,
            };
            resolution.source = "queue-track";
            return resolution;
        }

        if(!m_playlistHandler) {
            resolution.source = "no-playlist-handler";
            return resolution;
        }

        if(!resolution.activePlaylist) {
            resolution.source = "no-active-playlist";
            return resolution;
        }

        const bool playbackBackedByActivePlaylist = m_currentTrack.isValid() && m_currentTrack.playlistId.isValid()
                                                 && resolution.activePlaylist->id() == m_currentTrack.playlistId
                                                 && m_currentTrack.indexInPlaylist >= 0;

        if(playbackBackedByActivePlaylist) {
            resolution.previewIndex
                = resolution.activePlaylist->nextIndexFrom(m_currentTrack.indexInPlaylist, 1, m_playMode);
            resolution.upcoming.track
                = resolution.activePlaylist->playlistTrack(resolution.previewIndex).value_or(PlaylistTrack{});
            resolution.upcoming.isQueueTrack = false;
        }
        else {
            resolution.previewIndex          = resolution.activePlaylist->nextIndex(1, m_playMode);
            resolution.upcoming.track        = m_playlistHandler->peekRelativeTrack(m_playMode, 1);
            resolution.upcoming.isQueueTrack = false;
        }

        if(resolution.upcoming.track.isValid()) {
            resolution.source = playbackBackedByActivePlaylist ? "current-track-preview" : "active-playlist-preview";
            return resolution;
        }

        const int currentIndex  = resolution.activePlaylist->currentTrackIndex();
        const Track activeTrack = resolution.activePlaylist->currentTrack();

        if(resolution.activePlaylist->trackCount() <= 0) {
            resolution.source = "active-playlist-empty";
        }
        else if(currentIndex < 0) {
            resolution.source = "active-playlist-no-current";
        }
        else if(currentIndex >= resolution.activePlaylist->trackCount()) {
            resolution.source = "active-playlist-current-out-of-range";
        }
        else if(activeTrack.isValid() && !sameTrackIdentity(activeTrack, m_currentTrack.track)) {
            resolution.source = "active-playlist-current-mismatch";
        }
        else {
            resolution.source = "active-playlist-preview-invalid";
        }

        return resolution;
    }

    void emitUpcomingTrackChangedIfNeeded(const char* trigger = "state-refresh")
    {
        const UpcomingTrackResolution resolution = resolveUpcomingTrack();
        Player::UpcomingTrack upcoming           = resolution.upcoming;

        if(upcoming.track.isValid()) {
            const bool reuseLastUpcoming = m_lastUpcomingTrack.track == upcoming.track
                                        && m_lastUpcomingTrack.isQueueTrack == upcoming.isQueueTrack
                                        && m_lastUpcomingTrack.itemId != 0;
            upcoming.itemId              = reuseLastUpcoming ? m_lastUpcomingTrack.itemId : nextPlaybackItemId();
        }

        const UId activePlaylistId = resolution.activePlaylist ? resolution.activePlaylist->id() : UId{};
        const int activePlaylistCurrentIndex
            = resolution.activePlaylist ? resolution.activePlaylist->currentTrackIndex() : -1;
        const int activePlaylistTrackCount = resolution.activePlaylist ? resolution.activePlaylist->trackCount() : 0;
        const Track activePlaylistCurrentTrack
            = resolution.activePlaylist ? resolution.activePlaylist->currentTrack() : Track{};
        const bool activeCurrentMatchesPlaying = sameTrackIdentity(activePlaylistCurrentTrack, m_currentTrack.track);

        const bool upcomingChanged
            = m_lastUpcomingTrack.track != upcoming.track || m_lastUpcomingTrack.isQueueTrack != upcoming.isQueueTrack;
        const bool diagnosticChanged = m_lastUpcomingTrackSource != resolution.source
                                    || m_lastUpcomingPlaylistId != activePlaylistId
                                    || m_lastUpcomingPlaylistCurrentIndex != activePlaylistCurrentIndex
                                    || m_lastUpcomingPlaylistTrackCount != activePlaylistTrackCount
                                    || m_lastUpcomingPlaylistCurrentTrackId != activePlaylistCurrentTrack.id()
                                    || m_lastUpcomingPreviewIndex != resolution.previewIndex
                                    || m_lastUpcomingActiveCurrentMatchesPlaying != activeCurrentMatchesPlaying;

        if(upcomingChanged || diagnosticChanged) {
            qCDebug(PLAYER_CONTROLLER) << "Upcoming track resolution:" << "trigger=" << trigger
                                       << "currentTrackId=" << m_currentTrack.track.id()
                                       << "currentPlaylistId=" << m_currentTrack.playlistId
                                       << "currentTrackIndex=" << m_currentTrack.indexInPlaylist
                                       << "source=" << resolution.source
                                       << "upcomingTrackId=" << upcoming.track.track.id()
                                       << "upcomingItemId=" << upcoming.itemId
                                       << "isQueueTrack=" << upcoming.isQueueTrack
                                       << "queueCount=" << m_queue.trackCount()
                                       << "scheduledTrackId=" << m_scheduledTrack.track.id()
                                       << "activePlaylistId=" << activePlaylistId
                                       << "activePlaylistCurrentIndex=" << activePlaylistCurrentIndex
                                       << "activePlaylistTrackCount=" << activePlaylistTrackCount
                                       << "activePlaylistCurrentTrackId=" << activePlaylistCurrentTrack.id()
                                       << "activeCurrentMatchesPlaying=" << activeCurrentMatchesPlaying
                                       << "previewIndex=" << resolution.previewIndex;

            m_lastUpcomingTrackSource                 = resolution.source;
            m_lastUpcomingPlaylistId                  = activePlaylistId;
            m_lastUpcomingPlaylistCurrentIndex        = activePlaylistCurrentIndex;
            m_lastUpcomingPlaylistTrackCount          = activePlaylistTrackCount;
            m_lastUpcomingPlaylistCurrentTrackId      = activePlaylistCurrentTrack.id();
            m_lastUpcomingPreviewIndex                = resolution.previewIndex;
            m_lastUpcomingActiveCurrentMatchesPlaying = activeCurrentMatchesPlaying;
        }

        if(!upcomingChanged) {
            return;
        }

        m_lastUpcomingTrack = upcoming;
        emit m_self->upcomingTrackChanged(upcoming);
    }

    void requestTrackChange(const Player::TrackChangeRequest& request)
    {
        const auto requestWithId = withPlaybackItemId(request);
        m_pendingRequest         = requestWithId;
        m_pendingChangeContext   = requestWithId.context;
        emit m_self->trackChangeRequested(requestWithId);
    }

    void clearPendingRequest()
    {
        m_pendingRequest.reset();
        m_pendingChangeContext = {};
    }

    void loadScheduledTrack()
    {
        const PlaylistTrack scheduledTrack = m_scheduledTrack;

        if(auto* playlist = m_playlistHandler->playlistById(scheduledTrack.playlistId)) {
            m_playlistHandler->changeActivePlaylist(playlist);
            playlist->changeCurrentIndex(scheduledTrack.indexInPlaylist);
        }

        requestTrackChange({
            .track        = scheduledTrack,
            .context      = m_pendingChangeContext,
            .isQueueTrack = false,
        });
    }

    void updatePosition(uint64_t pos)
    {
        if(!m_seeking && pos > m_position) {
            m_timeListened += (pos - m_position);
        }
        m_seeking = false;

        m_position = pos;
    }

    bool updatePlaystate(Player::PlayState state)
    {
        if(std::exchange(m_playState, state) != state) {
            emit m_self->playStateChanged(state);
            emit m_self->playbackSnapshotChanged(m_self->playbackSnapshot());
            return true;
        }

        return false;
    }

    PlayerController* m_self;
    SettingsManager* m_settings;
    PlaylistHandler* m_playlistHandler{nullptr};

    Player::PlayState m_playState{Player::PlayState::Stopped};
    Playlist::PlayModes m_playMode;

    PlaylistTrack m_currentTrack;
    uint64_t m_totalDuration{0};
    uint64_t m_position{0};
    int m_bitrate{0};
    uint64_t m_lastPositionSecond{0};

    uint64_t m_timeListened{0};
    uint64_t m_playedThreshold{0};

    bool m_seeking{false};
    bool m_counted{false};
    bool m_isQueueTrack{false};
    bool m_stopCurrentSkip{false};
    bool m_hasLastPositionSecond{false};

    Player::TrackChangeContext m_pendingChangeContext;
    Player::TrackChangeContext m_lastChangeContext;
    std::optional<Player::TrackChangeRequest> m_pendingRequest;
    Player::UpcomingTrack m_lastUpcomingTrack;
    uint64_t m_currentItemId{0};
    uint64_t m_nextPlaybackItemId{1};
    const char* m_lastUpcomingTrackSource{"unknown"};
    UId m_lastUpcomingPlaylistId;
    int m_lastUpcomingPlaylistCurrentIndex{-1};
    int m_lastUpcomingPlaylistTrackCount{0};
    int m_lastUpcomingPlaylistCurrentTrackId{-1};
    int m_lastUpcomingPreviewIndex{-1};
    bool m_lastUpcomingActiveCurrentMatchesPlaying{false};

    PlaybackQueue m_queue;
    PlaylistTrack m_scheduledTrack;
};

PlayerController::PlayerController(SettingsManager* settings, PlaylistHandler* playlistHandler, QObject* parent)
    : QObject{parent}
    , p{std::make_unique<PlayerControllerPrivate>(this, settings, playlistHandler)}
{
    settings->subscribe<Settings::Core::PlayMode>(this, [this]() {
        const auto mode = static_cast<Playlist::PlayModes>(p->m_settings->value<Settings::Core::PlayMode>());
        if(std::exchange(p->m_playMode, mode) != mode) {
            emit playModeChanged(mode);
            p->emitUpcomingTrackChangedIfNeeded("play-mode-changed");
        }
    });
    settings->subscribe<Settings::Core::StopAfterCurrent>(
        this, [this]() { p->emitUpcomingTrackChangedIfNeeded("stop-after-current-changed"); });

    if(!playlistHandler) {
        return;
    }

    QObject::connect(playlistHandler, &PlaylistHandler::activePlaylistChanged, this,
                     [this]() { p->emitUpcomingTrackChangedIfNeeded("active-playlist-changed"); });
    QObject::connect(playlistHandler, &PlaylistHandler::tracksAdded, this, [this, playlistHandler](Playlist* playlist) {
        if(playlist == playlistHandler->activePlaylist()) {
            p->emitUpcomingTrackChangedIfNeeded("active-playlist-tracks-added");
        }
    });
    QObject::connect(playlistHandler, &PlaylistHandler::tracksPatched, this,
                     [this, playlistHandler](Playlist* playlist) {
                         if(playlist == playlistHandler->activePlaylist()) {
                             p->emitUpcomingTrackChangedIfNeeded("active-playlist-tracks-patched");
                         }
                     });
    QObject::connect(playlistHandler, &PlaylistHandler::tracksChanged, this,
                     [this, playlistHandler](Playlist* playlist) {
                         if(playlist == playlistHandler->activePlaylist()) {
                             p->emitUpcomingTrackChangedIfNeeded("active-playlist-tracks-changed");
                         }
                     });
    QObject::connect(playlistHandler, &PlaylistHandler::tracksUpdated, this,
                     [this, playlistHandler](Playlist* playlist) {
                         if(playlist == playlistHandler->activePlaylist()) {
                             p->emitUpcomingTrackChangedIfNeeded("active-playlist-tracks-updated");
                         }
                     });
    QObject::connect(playlistHandler, &PlaylistHandler::tracksRemoved, this,
                     [this, playlistHandler](Playlist* playlist) {
                         if(playlist == playlistHandler->activePlaylist()) {
                             p->emitUpcomingTrackChangedIfNeeded("active-playlist-tracks-removed");
                         }
                     });

    QObject::connect(playlistHandler, &PlaylistHandler::activePlaylistDeleted, this, [this]() {
        if(p->m_settings->value<Settings::Core::StopIfActivePlaylistDeleted>()) {
            stop();
        }
    });

    QObject::connect(playlistHandler, &PlaylistHandler::restoreCurrentTrackRequested, this,
                     [this](const PlaylistTrack& track) { commitCurrentTrack(track); });

    QObject::connect(playlistHandler, &PlaylistHandler::playlistReferencesRemapRequested, this,
                     [this](const UId& fromPlaylistId, const UId& toPlaylistId) {
                         remapPlaylistReferences(fromPlaylistId, toPlaylistId);
                     });
}

PlayerController::~PlayerController() = default;

void PlayerController::reset()
{
    p->m_currentTrack  = {};
    p->m_currentItemId = 0;
    p->m_position      = 0;
    p->clearPendingRequest();
    p->updateBitrate(0);

    p->emitPositionSignals(0);
    emit playbackSnapshotChanged(playbackSnapshot());
    p->emitUpcomingTrackChangedIfNeeded("controller-reset");
}

void PlayerController::play()
{
    if(!p->m_currentTrack.isValid() && !p->m_pendingRequest.has_value()) {
        if(p->m_scheduledTrack.isValid()) {
            p->loadScheduledTrack();
        }
        else if(!p->m_queue.empty()) {
            const auto nextTrack = p->m_queue.nextTrack();

            if(p->m_playlistHandler) {
                if(Playlist* playlist = p->m_playlistHandler->activePlaylist()) {
                    if(p->m_queue.trackCount() == 1 && p->m_settings->value<Settings::Core::FollowPlaybackQueue>()) {
                        if(playlist->id() != nextTrack.playlistId && nextTrack.playlistId.isValid()) {
                            p->m_playlistHandler->changeActivePlaylist(nextTrack.playlistId);
                        }

                        const int index = nextTrack.indexInPlaylist;
                        if(playlist->id() != nextTrack.playlistId) {
                            playlist = p->m_playlistHandler->playlistById(nextTrack.playlistId);
                        }

                        if(index != -1) {
                            playlist->changeCurrentIndex(index);
                        }
                    }
                }
            }

            p->requestTrackChange({
                .track        = p->m_queue.nextTrack(),
                .context      = p->m_pendingChangeContext,
                .isQueueTrack = true,
            });
        }
        else if(p->m_playlistHandler) {
            const PlaylistTrack track = p->m_playlistHandler->advanceRelativeTrack(p->m_playMode, 1);
            p->requestTrackChange({
                .track        = track,
                .context      = p->m_pendingChangeContext,
                .isQueueTrack = false,
            });
        }
    }

    if(p->m_currentTrack.isValid() || p->m_pendingRequest.has_value()) {
        if(p->updatePlaystate(Player::PlayState::Playing)) {
            emit transportPlayRequested();
        }
    }
    else {
        p->m_currentTrack = {};
        emit playbackSnapshotChanged(playbackSnapshot());
    }
}

void PlayerController::playPause()
{
    switch(p->m_playState) {
        case(Player::PlayState::Playing):
            pause();
            break;
        case(Player::PlayState::Paused):
        case(Player::PlayState::Stopped):
            play();
            break;
        default:
            break;
    }
}

void PlayerController::pause()
{
    if(p->updatePlaystate(Player::PlayState::Paused)) {
        emit transportPauseRequested();
    }
}

void PlayerController::previous()
{
    if(p->m_settings->value<Settings::Core::RewindPreviousTrack>() && currentPosition() > 5000) {
        seek(0);
        return;
    }

    // Temporarily disable repeating track when user clicks 'previous'.
    const auto playMode = p->m_playMode;
    p->m_playMode &= ~Playlist::RepeatTrack;

    if(p->m_playlistHandler) {
        const PlaylistTrack track = p->m_playlistHandler->advanceRelativeTrack(p->m_playMode, -1);
        changeCurrentTrack(track, {Player::AdvanceReason::ManualPrevious, true});
    }
    else {
        p->m_currentTrack = {};
    }

    if(p->m_currentTrack.isValid() || p->m_pendingRequest.has_value()) {
        play();
    }

    p->m_playMode = playMode;
}

void PlayerController::next()
{
    // Temporarily disable repeating track and 'stop after current' when user clicks 'next'.
    const auto playMode = p->m_playMode;
    p->m_playMode &= ~Playlist::RepeatTrack;
    p->m_stopCurrentSkip = true;

    advance(Player::AdvanceReason::ManualNext);

    p->m_playMode        = playMode;
    p->m_stopCurrentSkip = false;
}

void PlayerController::advance(Player::AdvanceReason reason)
{
    const bool userInitiated
        = reason == Player::AdvanceReason::ManualNext || reason == Player::AdvanceReason::ManualPrevious;
    p->m_pendingChangeContext = {reason, userInitiated};

    if(!p->m_stopCurrentSkip && p->m_settings->value<Settings::Core::StopAfterCurrent>()) {
        if(p->m_settings->value<Settings::Core::ResetStopAfterCurrent>()) {
            p->m_settings->set<Settings::Core::StopAfterCurrent>(false);
        }
        reset();
        stop();
        return;
    }

    if(reason == Player::AdvanceReason::NaturalEnd && !hasNextTrack()) {
        // Preserve current track context when playback naturally ends with no
        // upcoming track so stopped seeks and play still works.
        p->m_pendingChangeContext = {};
        stop();
        return;
    }

    if(p->m_scheduledTrack.isValid()) {
        p->loadScheduledTrack();
    }
    else if(p->m_queue.empty() && p->m_playlistHandler) {
        if(p->m_isQueueTrack && p->m_settings->value<Settings::Core::PlaybackQueueStopWhenFinished>()) {
            reset();
            stop();
            return;
        }

        const PlaylistTrack track = p->m_playlistHandler->advanceRelativeTrack(p->m_playMode, 1);
        p->requestTrackChange({
            .track        = track,
            .context      = p->m_pendingChangeContext,
            .isQueueTrack = false,
        });
    }
    else {
        if(!p->m_queue.empty()) {
            p->requestTrackChange({
                .track        = p->m_queue.nextTrack(),
                .context      = p->m_pendingChangeContext,
                .isQueueTrack = true,
            });
        }
    }

    if(p->m_pendingRequest.has_value()) {
        play();
    }
    else {
        stop();
    }
}

Player::TrackChangeContext PlayerController::lastTrackChangeContext() const
{
    return p->m_lastChangeContext;
}

void PlayerController::stop()
{
    if(p->updatePlaystate(Player::PlayState::Stopped)) {
        p->m_position = 0;
        p->emitPositionSignals(0);
        emit transportStopRequested();
    }
}

void PlayerController::syncPlayStateFromEngine(Player::PlayState state)
{
    switch(state) {
        case Player::PlayState::Playing:
            p->updatePlaystate(Player::PlayState::Playing);
            break;
        case Player::PlayState::Paused:
            p->updatePlaystate(Player::PlayState::Paused);
            break;
        case Player::PlayState::Stopped:
            if(p->updatePlaystate(Player::PlayState::Stopped)) {
                p->m_position = 0;
                p->emitPositionSignals(0);
            }
            break;
    }
}

void PlayerController::setCurrentPosition(uint64_t ms)
{
    p->updatePosition(ms);

    if(!p->m_counted && p->m_timeListened >= p->m_playedThreshold) {
        p->m_counted = true;
        if(p->m_currentTrack.isValid()) {
            qCDebug(PLAYER_CONTROLLER) << "Track reached played threshold:" << "id=" << p->m_currentTrack.track.id()
                                       << "path=" << p->m_currentTrack.track.uniqueFilepath()
                                       << "timeListened=" << p->m_timeListened
                                       << "playedThreshold=" << p->m_playedThreshold
                                       << "playCount=" << p->m_currentTrack.track.playCount();
            emit trackPlayed(p->m_currentTrack.track);
        }
    }

    p->emitPositionSignals(ms);
}

void PlayerController::setBitrate(int bitrate)
{
    if(p->updateBitrate(bitrate)) {
        emit playbackSnapshotChanged(playbackSnapshot());
    }
}

void PlayerController::changeCurrentTrack(const Track& track)
{
    changeCurrentTrack(PlaylistTrack{.track = track, .playlistId = {}});
}

void PlayerController::changeCurrentTrack(const PlaylistTrack& track, const Player::TrackChangeContext& context)
{
    if(!track.isValid()) {
        return;
    }

    p->requestTrackChange({
        .track        = track,
        .context      = context,
        .isQueueTrack = false,
    });
}

void PlayerController::commitCurrentTrack(const Track& track)
{
    commitCurrentTrack(PlaylistTrack{.track = track, .playlistId = {}});
}

void PlayerController::commitCurrentTrack(const Player::TrackChangeRequest& request)
{
    const auto requestWithId = p->withPlaybackItemId(request);
    if(!requestWithId.track.isValid()) {
        reset();
        return;
    }

    Player::TrackChangeContext commitContext = requestWithId.context;
    bool isQueueTrack                        = requestWithId.isQueueTrack;
    uint64_t itemId                          = requestWithId.itemId;

    if(p->m_pendingRequest.has_value() && p->m_pendingRequest->track == requestWithId.track
       && p->m_pendingRequest->itemId == requestWithId.itemId) {
        isQueueTrack  = p->m_pendingRequest->isQueueTrack;
        commitContext = p->m_pendingRequest->context;
        itemId        = p->m_pendingRequest->itemId;
    }

    p->changeTrack(requestWithId.track, commitContext);
    p->m_currentItemId = itemId;
    p->m_isQueueTrack  = isQueueTrack;
    p->updateBitrate(0);
    p->clearPendingRequest();
    p->m_lastUpcomingTrack = {};

    if(isQueueTrack) {
        const auto removedTracks = p->m_queue.removeTracks({requestWithId.track});
        if(!removedTracks.empty()) {
            emit tracksDequeued(removedTracks);
        }
    }

    p->m_settings->set<Settings::Core::ActiveTrack>(QVariant::fromValue(p->m_currentTrack.track));
    p->m_settings->set<Settings::Core::ActiveTrackId>(p->m_currentTrack.track.id());

    emit currentTrackChanged(p->m_currentTrack.track);
    emit playlistTrackChanged(p->m_currentTrack);
    emit playbackSnapshotChanged(playbackSnapshot());
    p->emitUpcomingTrackChangedIfNeeded("track-committed");
}

void PlayerController::commitCurrentTrack(const PlaylistTrack& track, const Player::TrackChangeContext& context)
{
    commitCurrentTrack(Player::TrackChangeRequest{
        .track        = track,
        .context      = context,
        .isQueueTrack = false,
        .itemId       = p->nextPlaybackItemId(),
    });
}

void PlayerController::updateCurrentTrack(const Track& track)
{
    if(track.uniqueFilepath() == p->m_currentTrack.track.uniqueFilepath()
       && track.duration() == p->m_currentTrack.track.duration()) {
        p->m_currentTrack.track = track;
        emit currentTrackUpdated(track);
    }
}

void PlayerController::updateCurrentTrackPlaylist(const UId& playlistId)
{
    if(std::exchange(p->m_currentTrack.playlistId, playlistId) != playlistId) {
        emit playlistTrackChanged(p->m_currentTrack);
    }
}

void PlayerController::updateCurrentTrackIndex(int index)
{
    if(std::exchange(p->m_currentTrack.indexInPlaylist, index) != index) {
        emit playlistTrackChanged(p->m_currentTrack);
    }
}

void PlayerController::scheduleNextTrack(const PlaylistTrack& track)
{
    p->m_scheduledTrack = track;
    p->emitUpcomingTrackChangedIfNeeded("track-scheduled");
}

void PlayerController::remapPlaylistReferences(const UId& fromPlaylistId, const UId& toPlaylistId)
{
    if(!fromPlaylistId.isValid() || !toPlaylistId.isValid() || fromPlaylistId == toPlaylistId) {
        return;
    }

    auto queueTracks = p->m_queue.tracks();
    bool updated     = false;
    for(auto& track : queueTracks) {
        if(track.playlistId == fromPlaylistId) {
            track.playlistId = toPlaylistId;
            updated          = true;
        }
    }

    if(updated) {
        replaceTracks(queueTracks);
    }

    if(p->m_currentTrack.playlistId == fromPlaylistId) {
        updateCurrentTrackPlaylist(toPlaylistId);
    }

    p->emitUpcomingTrackChangedIfNeeded("playlist-references-remapped");
}

Track PlayerController::upcomingTrack() const
{
    return p->resolveUpcomingTrack().upcoming.track.track;
}

PlaylistTrack PlayerController::upcomingPlaylistTrack() const
{
    return p->resolveUpcomingTrack().upcoming.track;
}

bool PlayerController::hasNextTrack() const
{
    return upcomingTrack().isValid();
}

bool PlayerController::hasPreviousTrack() const
{
    if(!p->m_playlistHandler) {
        return false;
    }

    return p->m_playlistHandler->peekRelativeTrack(p->m_playMode, -1).isValid();
}

Player::PlaybackSnapshot PlayerController::playbackSnapshot() const
{
    return {
        .playState       = p->m_playState,
        .track           = p->m_currentTrack.track,
        .playlistId      = p->m_currentTrack.playlistId,
        .indexInPlaylist = p->m_currentTrack.indexInPlaylist,
        .positionMs      = p->m_position,
        .durationMs      = p->m_totalDuration,
        .bitrate         = p->m_bitrate,
        .isQueueTrack    = p->m_isQueueTrack,
    };
}

PlaybackQueue PlayerController::playbackQueue() const
{
    return p->m_queue;
}

int PlayerController::queuedTracksCount() const
{
    return p->m_queue.trackCount();
}

void PlayerController::setPlayMode(Playlist::PlayModes mode)
{
    p->m_settings->set<Settings::Core::PlayMode>(static_cast<int>(mode));
}

void PlayerController::seek(uint64_t ms)
{
    if(p->m_totalDuration < 100) {
        return;
    }

    if(ms >= p->m_totalDuration - 100) {
        next();
        return;
    }

    if(std::exchange(p->m_position, ms) != ms) {
        p->m_seeking = true;
        emit positionMoved(ms);
    }
}

void PlayerController::seekForward(uint64_t delta)
{
    seek(p->m_position + delta);
}

void PlayerController::seekBackward(uint64_t delta)
{
    if(delta > p->m_position) {
        seek(0);
    }
    else {
        seek(p->m_position - delta);
    }
}

void PlayerController::startPlayback(const UId& playlistId)
{
    if(!p->m_playlistHandler) {
        return;
    }

    if(auto* playlist = p->m_playlistHandler->playlistById(playlistId)) {
        p->m_playlistHandler->changeActivePlaylist(playlistId);
        playlist->reset();

        const PlaylistTrack currentTrack = p->m_playlistHandler->currentTrack();
        changeCurrentTrack(currentTrack, {Player::AdvanceReason::ManualSelection, true});
        play();
    }
}

void PlayerController::startPlayback(Playlist* playlist)
{
    if(!playlist || !p->m_playlistHandler) {
        return;
    }

    if(!playlist->id().isValid() || p->m_playlistHandler->playlistById(playlist->id()) != playlist) {
        return;
    }

    p->m_playlistHandler->changeActivePlaylist(playlist);
    playlist->reset();
    if(playlist->currentTrackIndex() < 0) {
        playlist->changeCurrentIndex(0);
    }

    const PlaylistTrack currentTrack = p->m_playlistHandler->currentTrack();
    changeCurrentTrack(currentTrack, {Player::AdvanceReason::ManualSelection, true});
    play();
}

Player::PlayState PlayerController::playState() const
{
    return p->m_playState;
}

Playlist::PlayModes PlayerController::playMode() const
{
    return p->m_playMode;
}

uint64_t PlayerController::currentPosition() const
{
    return p->m_position;
}

int PlayerController::bitrate() const
{
    return p->m_bitrate;
}

Track PlayerController::currentTrack() const
{
    return p->m_currentTrack.track;
}

int PlayerController::currentTrackId() const
{
    return p->m_currentTrack.isValid() ? p->m_currentTrack.track.id() : -1;
}

bool PlayerController::currentIsQueueTrack() const
{
    return p->m_isQueueTrack;
}

PlaylistTrack PlayerController::currentPlaylistTrack() const
{
    return p->m_currentTrack;
}

void PlayerController::queueTrack(const Track& track)
{
    queueTrack(PlaylistTrack{.track = track, .playlistId = {}});
}

void PlayerController::queueTrack(const PlaylistTrack& track)
{
    queueTracks({track});
}

void PlayerController::queueTracks(const TrackList& tracks)
{
    if(tracks.empty()) {
        return;
    }

    QueueTracks tracksToQueue;
    for(const Track& track : tracks) {
        tracksToQueue.emplace_back(track);
    }

    queueTracks(tracksToQueue);
}

void PlayerController::queueTracks(const QueueTracks& tracks)
{
    if(tracks.empty()) {
        return;
    }

    QueueTracks tracksToAdd{tracks};

    const int freeTracks = p->m_queue.freeSpace();
    if(std::cmp_greater_equal(tracks.size(), freeTracks)) {
        tracksToAdd = {tracks.begin(), tracks.begin() + freeTracks};
    }

    const int index = p->m_queue.trackCount();

    p->m_queue.addTracks(tracksToAdd);
    emit tracksQueued(tracksToAdd, index);
    p->emitUpcomingTrackChangedIfNeeded("queue-tracks-added");
}

void PlayerController::queueTrackNext(const Track& track)
{
    queueTrackNext(PlaylistTrack{.track = track, .playlistId = {}});
}

void PlayerController::queueTrackNext(const PlaylistTrack& track)
{
    queueTracksNext({track});
}

void PlayerController::queueTracksNext(const TrackList& tracks)
{
    if(tracks.empty()) {
        return;
    }

    QueueTracks tracksToQueue;
    for(const Track& track : tracks) {
        tracksToQueue.emplace_back(track);
    }

    queueTracksNext(tracksToQueue);
}

void PlayerController::queueTracksNext(const QueueTracks& tracks)
{
    if(tracks.empty()) {
        return;
    }

    QueueTracks tracksToAdd{tracks};

    const int freeTracks = p->m_queue.freeSpace();
    if(std::cmp_greater_equal(tracks.size(), freeTracks)) {
        tracksToAdd = {tracks.begin(), tracks.begin() + freeTracks};
    }

    p->m_queue.addTracks(tracksToAdd, 0);
    emit trackQueueChanged({}, p->m_queue.tracks());
    p->emitUpcomingTrackChangedIfNeeded("queue-tracks-added-next");
}

void PlayerController::dequeueTrack(const Track& track)
{
    dequeueTrack(PlaylistTrack{.track = track, .playlistId = {}});
}

void PlayerController::dequeueTrack(const PlaylistTrack& track)
{
    dequeueTracks({track});
}

void PlayerController::dequeueTracks(const TrackList& tracks)
{
    if(tracks.empty()) {
        return;
    }

    QueueTracks tracksToDequeue;
    for(const Track& track : tracks) {
        tracksToDequeue.emplace_back(track);
    }

    dequeueTracks(tracksToDequeue);
}

void PlayerController::dequeueTracks(const QueueTracks& tracks)
{
    if(tracks.empty()) {
        return;
    }

    const auto removedTracks = p->m_queue.removeTracks(tracks);
    if(!removedTracks.empty()) {
        emit tracksDequeued(removedTracks);
        p->emitUpcomingTrackChangedIfNeeded("queue-tracks-removed");
    }
}

void PlayerController::dequeueTracks(const std::vector<int>& indexes)
{
    if(indexes.empty()) {
        return;
    }

    PlaylistIndexes dequeuedIndexes;

    std::vector<int> sortedIndexes{indexes};
    std::ranges::sort(sortedIndexes, std::greater{}); // Reverse sort

    auto tracks      = p->m_queue.tracks();
    const auto count = static_cast<int>(tracks.size());
    for(const int index : sortedIndexes) {
        if(index >= 0 && index < count) {
            const auto track = p->m_queue.track(index);
            dequeuedIndexes[track.playlistId].emplace_back(track.indexInPlaylist);
            tracks.erase(tracks.begin() + index);
        }
    }

    p->m_queue.replaceTracks(tracks);

    if(!dequeuedIndexes.empty()) {
        emit trackIndexesDequeued(dequeuedIndexes);
        p->emitUpcomingTrackChangedIfNeeded("queue-track-indexes-removed");
    }
}

void PlayerController::replaceTracks(const TrackList& tracks)
{
    QueueTracks tracksToQueue;
    for(const Track& track : tracks) {
        tracksToQueue.emplace_back(track);
    }

    replaceTracks(tracksToQueue);
}

void PlayerController::replaceTracks(const QueueTracks& tracks)
{
    QueueTracks removed;

    const auto currentTracks = p->m_queue.tracks();
    const std::set<PlaylistTrack> newTracks{tracks.cbegin(), tracks.cend()};

    std::ranges::copy_if(currentTracks, std::back_inserter(removed),
                         [&newTracks](const PlaylistTrack& oldTrack) { return !newTracks.contains(oldTrack); });

    p->m_queue.replaceTracks(tracks);

    emit trackQueueChanged(removed, tracks);
    p->emitUpcomingTrackChangedIfNeeded("queue-replaced");
}

void PlayerController::clearPlaylistQueue(const UId& playlistId)
{
    const auto removedTracks = p->m_queue.removePlaylistTracks(playlistId);
    if(!removedTracks.empty()) {
        emit tracksDequeued(removedTracks);
        p->emitUpcomingTrackChangedIfNeeded("playlist-queue-cleared");
    }
}

void PlayerController::clearQueue()
{
    const auto removedTracks = p->m_queue.tracks();
    p->m_queue.clear();
    if(!removedTracks.empty()) {
        emit tracksDequeued(removedTracks);
    }
    p->emitUpcomingTrackChangedIfNeeded("queue-cleared");
}

} // namespace Fooyin

#include "core/player/moc_playercontroller.cpp"
