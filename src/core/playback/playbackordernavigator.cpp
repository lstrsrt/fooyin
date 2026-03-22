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

#include "playbackordernavigator.h"

#include "core/playlist/playlisthandler.h"

#include <core/player/playbackqueue.h>

#include <core/coresettings.h>
#include <core/track.h>
#include <utils/settings/settingsmanager.h>

#include <algorithm>

namespace Fooyin {
PlaybackOrderNavigator::PlaybackOrderNavigator(SettingsManager* settings, PlaylistHandler* playlistHandler,
                                               PlaybackQueue* queue, PlaybackOrderState state)
    : m_settings{settings}
    , m_playlistHandler{playlistHandler}
    , m_queue{queue}
    , m_state{state}
{ }

Playlist* PlaybackOrderNavigator::playbackPlaylist() const
{
    if(!m_playlistHandler || *m_state.isQueueTrack || !m_state.currentTrack->isValid()
       || !m_state.currentTrack->playlistId.isValid() || m_state.currentTrack->indexInPlaylist < 0) {
        return nullptr;
    }

    auto* playlist = m_playlistHandler->playlistById(m_state.currentTrack->playlistId);
    if(!playlist || m_state.currentTrack->indexInPlaylist >= playlist->trackCount()) {
        return nullptr;
    }

    const auto playlistTrack = playlist->playlistTrack(m_state.currentTrack->indexInPlaylist);
    if(!playlistTrack.has_value() || !playlistTrack->track.sameIdentityAs(m_state.currentTrack->track)) {
        return nullptr;
    }

    return playlist;
}

PlaylistTrack PlaybackOrderNavigator::previewPlaybackRelativeTrack(int delta) const
{
    if(auto* playlist = playbackPlaylist()) {
        const int nextIndex = playlist->nextIndexFrom(m_state.currentTrack->indexInPlaylist, delta, *m_state.playMode);
        return playlist->playlistTrack(nextIndex).value_or(PlaylistTrack{});
    }

    if(!m_playlistHandler) {
        return {};
    }

    return m_playlistHandler->peekRelativeTrack(*m_state.playMode, delta);
}

PlaylistTrack PlaybackOrderNavigator::advancePlaybackRelativeTrack(int delta)
{
    if(auto* playlist = playbackPlaylist()) {
        const int nextIndex = playlist->nextIndexFrom(m_state.currentTrack->indexInPlaylist, delta, *m_state.playMode);
        if(nextIndex < 0 || nextIndex >= playlist->trackCount()) {
            playlist->changeCurrentIndex(-1);
            return {};
        }

        playlist->changeCurrentIndex(nextIndex);
        return playlist->playlistTrack(nextIndex).value_or(PlaylistTrack{});
    }

    if(!m_playlistHandler) {
        return {};
    }

    return m_playlistHandler->advanceRelativeTrack(*m_state.playMode, delta);
}

PlaylistTrack PlaybackOrderNavigator::restartPlaylistFromBeginning()
{
    Playlist* playlist{playbackPlaylist()};

    if(!playlist && m_playlistHandler) {
        playlist = m_playlistHandler->activePlaylist();
    }

    if(!playlist || playlist->trackCount() <= 0) {
        return {};
    }

    if(m_playlistHandler && m_playlistHandler->activePlaylist() != playlist) {
        m_playlistHandler->changeActivePlaylist(playlist);
    }

    const int restartIndex = playlist->firstIndex(*m_state.playMode);
    const int startIndex   = restartIndex >= 0 ? restartIndex : 0;

    playlist->changeCurrentIndex(startIndex);
    return playlist->playlistTrack(startIndex).value_or(PlaylistTrack{});
}

PlaylistTrack PlaybackOrderNavigator::restartPlaylistFromEnd()
{
    Playlist* playlist{playbackPlaylist()};

    if(!playlist && m_playlistHandler) {
        playlist = m_playlistHandler->activePlaylist();
    }

    if(!playlist || playlist->trackCount() <= 0) {
        return {};
    }

    if(m_playlistHandler && m_playlistHandler->activePlaylist() != playlist) {
        m_playlistHandler->changeActivePlaylist(playlist);
    }

    const int restartIndex = playlist->lastIndex(*m_state.playMode);
    const int endIndex     = restartIndex >= 0 ? restartIndex : std::max(playlist->trackCount() - 1, 0);

    playlist->changeCurrentIndex(endIndex);
    return playlist->playlistTrack(endIndex).value_or(PlaylistTrack{});
}

PlaylistTrack PlaybackOrderNavigator::restartPlaylist(RestartTarget target)
{
    switch(target) {
        case RestartTarget::Beginning:
            return restartPlaylistFromBeginning();
        case RestartTarget::End:
            return restartPlaylistFromEnd();
        case RestartTarget::None:
        default:
            return {};
    }
}

void PlaybackOrderNavigator::activatePlaylistTrack(const PlaylistTrack& track)
{
    if(!m_playlistHandler || !track.playlistId.isValid()) {
        return;
    }

    if(auto* playlist = m_playlistHandler->playlistById(track.playlistId)) {
        if(m_playlistHandler->activePlaylist() != playlist) {
            m_playlistHandler->changeActivePlaylist(playlist);
        }

        if(track.indexInPlaylist >= 0) {
            playlist->changeCurrentIndex(track.indexInPlaylist);
        }
    }
}

PlaylistTrack PlaybackOrderNavigator::followQueuedTrackIndex(int delta)
{
    if(!m_playlistHandler) {
        return {};
    }

    if(auto* playlist = m_playlistHandler->activePlaylist()) {
        const int nextIndex = playlist->nextIndexFrom(m_state.currentTrack->indexInPlaylist, delta, *m_state.playMode);
        if(const auto track = playlist->playlistTrack(nextIndex)) {
            return *track;
        }
    }

    return {};
}

std::optional<PlaybackOrderNavigator::RequestedTrack> PlaybackOrderNavigator::selectScheduledTrack()
{
    if(!m_state.scheduledTrack->isValid()) {
        return {};
    }

    activatePlaylistTrack(*m_state.scheduledTrack);
    return RequestedTrack{
        .track        = *m_state.scheduledTrack,
        .isQueueTrack = false,
    };
}

std::optional<PlaybackOrderNavigator::RequestedTrack> PlaybackOrderNavigator::selectPlaybackOrderTrack(int delta)
{
    if(delta <= 0) {
        if(m_playlistHandler) {
            return RequestedTrack{
                .track        = advancePlaybackRelativeTrack(delta),
                .isQueueTrack = false,
            };
        }
        return {};
    }

    if(m_queue->empty() && m_playlistHandler) {
        if(m_settings->value<Settings::Core::FollowPlaybackQueue>() && *m_state.isQueueTrack) {
            return RequestedTrack{
                .track        = followQueuedTrackIndex(delta),
                .isQueueTrack = false,
            };
        }

        return RequestedTrack{
            .track        = advancePlaybackRelativeTrack(delta),
            .isQueueTrack = false,
        };
    }

    if(!m_queue->empty()) {
        return RequestedTrack{
            .track        = m_queue->nextTrack(),
            .isQueueTrack = true,
        };
    }

    return {};
}

std::optional<PlaybackOrderNavigator::RequestedTrack> PlaybackOrderNavigator::selectPlayFromIdleState()
{
    if(auto scheduledTrack = selectScheduledTrack()) {
        return scheduledTrack;
    }

    if(!m_queue->empty()) {
        return RequestedTrack{
            .track        = m_queue->nextTrack(),
            .isQueueTrack = true,
        };
    }

    if(m_playlistHandler) {
        return RequestedTrack{
            .track        = m_playlistHandler->advanceRelativeTrack(*m_state.playMode, 1),
            .isQueueTrack = false,
        };
    }

    return {};
}
} // namespace Fooyin
