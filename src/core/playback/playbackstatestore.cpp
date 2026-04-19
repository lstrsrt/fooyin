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

#include "playbackstatestore.h"

#include <core/coresettings.h>
#include <utils/enum.h>

using namespace Qt::StringLiterals;

constexpr auto ActivePlaylistDbIdKey = "Playlist/ActiveId"_L1;
constexpr auto ActiveTrackIndexKey   = "Playlist/ActiveTrackIndex"_L1;
constexpr auto LastPlaybackPosition  = "Player/LastPosition"_L1;
constexpr auto LastPlaybackState     = "Player/LastState"_L1;

namespace Fooyin {
void PlaybackStateStore::saveActivePlaylistDbId(int dbId) const
{
    FyStateSettings stateSettings;
    stateSettings.setValue(ActivePlaylistDbIdKey, dbId);
}

void PlaybackStateStore::clearActivePlaylistDbId() const
{
    FyStateSettings stateSettings;
    stateSettings.remove(ActivePlaylistDbIdKey);
}

std::optional<int> PlaybackStateStore::activePlaylistDbId() const
{
    const FyStateSettings stateSettings;
    if(stateSettings.contains(ActivePlaylistDbIdKey)) {
        return stateSettings.value(ActivePlaylistDbIdKey).toInt();
    }

    return {};
}

void PlaybackStateStore::saveActiveTrackIndex(int index) const
{
    FyStateSettings stateSettings;
    stateSettings.setValue(ActiveTrackIndexKey, index);
}

void PlaybackStateStore::clearActiveTrackIndex() const
{
    FyStateSettings stateSettings;
    stateSettings.remove(ActiveTrackIndexKey);
}

std::optional<int> PlaybackStateStore::activeTrackIndex() const
{
    const FyStateSettings stateSettings;
    if(stateSettings.contains(ActiveTrackIndexKey)) {
        return stateSettings.value(ActiveTrackIndexKey).toInt();
    }

    return {};
}

void PlaybackStateStore::savePlaybackPosition(uint64_t position) const
{
    FyStateSettings stateSettings;
    stateSettings.setValue(LastPlaybackPosition, QVariant::fromValue(position));
}

void PlaybackStateStore::clearPlaybackPosition() const
{
    FyStateSettings stateSettings;
    stateSettings.remove(LastPlaybackPosition);
}

std::optional<uint64_t> PlaybackStateStore::playbackPosition() const
{
    const FyStateSettings stateSettings;
    if(stateSettings.contains(LastPlaybackPosition)) {
        return stateSettings.value(LastPlaybackPosition).value<uint64_t>();
    }

    return {};
}

void PlaybackStateStore::savePlaybackState(Player::PlayState state) const
{
    FyStateSettings stateSettings;
    stateSettings.setValue(LastPlaybackState, Utils::Enum::toString(state));
}

void PlaybackStateStore::clearPlaybackState() const
{
    FyStateSettings stateSettings;
    stateSettings.remove(LastPlaybackState);
}

std::optional<Player::PlayState> PlaybackStateStore::playbackState() const
{
    const FyStateSettings stateSettings;
    if(stateSettings.contains(LastPlaybackState)) {
        return Utils::Enum::fromString<Player::PlayState>(stateSettings.value(LastPlaybackState).toString());
    }

    return {};
}
} // namespace Fooyin
