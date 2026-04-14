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

#include <array>

#include <QString>

namespace Fooyin::ContextMenuIds {
namespace TrackSelection {
constexpr auto ArtworkSearchSeparator = "Fooyin.Menu.Artwork.SearchSeparator";
constexpr auto ArtworkAttachSeparator = "Fooyin.Menu.Artwork.AttachSeparator";
} // namespace TrackSelection

namespace Playlist {
constexpr auto Play               = "Fooyin.Context.Playlist.Play";
constexpr auto StopAfterThis      = "Fooyin.Context.Playlist.StopAfterThis";
constexpr auto PlaybackSeparator  = "Fooyin.Context.Playlist.Playback.Separator";
constexpr auto Remove             = "Fooyin.Context.Playlist.Remove";
constexpr auto MutateSeparator    = "Fooyin.Context.Playlist.Mutate.Separator";
constexpr auto Crop               = "Fooyin.Context.Playlist.Crop";
constexpr auto Sort               = "Fooyin.Context.Playlist.Sort";
constexpr auto Clipboard          = "Fooyin.Context.Playlist.Clipboard";
constexpr auto ClipboardSeparator = "Fooyin.Context.Playlist.Clipboard.Separator";
constexpr auto Presets            = "Fooyin.Context.Playlist.Presets";
constexpr auto PresetsSeparator   = "Fooyin.Context.Playlist.Presets.Separator";
constexpr auto Queue              = "Fooyin.Context.Playlist.Queue";
constexpr auto TrackActions       = "Fooyin.Context.Playlist.TrackActions";

struct Item
{
    const char* id;
    const char* title;
    bool isSeparator;
};

constexpr auto DefaultItems = std::to_array<Item>({
    {.id = Play, .title = "Play", .isSeparator = false},
    {.id = StopAfterThis, .title = "Stop after this", .isSeparator = false},
    {.id = PlaybackSeparator, .title = nullptr, .isSeparator = true},
    {.id = Remove, .title = "Remove", .isSeparator = false},
    {.id = Crop, .title = "Crop", .isSeparator = false},
    {.id = Sort, .title = "Sort", .isSeparator = false},
    {.id = MutateSeparator, .title = nullptr, .isSeparator = true},
    {.id = Clipboard, .title = "Clipboard", .isSeparator = false},
    {.id = ClipboardSeparator, .title = nullptr, .isSeparator = true},
    {.id = Presets, .title = "Presets", .isSeparator = false},
    {.id = PresetsSeparator, .title = nullptr, .isSeparator = true},
    {.id = Queue, .title = "Queue", .isSeparator = false},
    {.id = TrackActions, .title = "Track actions", .isSeparator = false},
});

inline bool isBuiltInSeparatorId(const QString& id)
{
    return std::ranges::any_of(
        DefaultItems, [&id](const auto& item) { return item.isSeparator && id == QString::fromUtf8(item.id); });
}
} // namespace Playlist
} // namespace Fooyin::ContextMenuIds
