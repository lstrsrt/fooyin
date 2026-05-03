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

#include <gui/guiconstants.h>
#include <gui/settings/context/staticcontextmenu.h>

#include <array>

namespace Fooyin::ContextMenuIds {
using Item = StaticContextMenu::Item;
using StaticContextMenu::defaultLayoutIds;
using StaticContextMenu::isBuiltInSeparatorId;

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

constexpr auto DefaultItems = std::to_array<Item>({
    {.id          = Play,
     .title       = {.context = "PlaylistWidget", .sourceText = QT_TRANSLATE_NOOP("PlaylistWidget", "Play")},
     .isSeparator = false},
    {.id          = StopAfterThis,
     .title       = {.context = "PlaylistWidget", .sourceText = QT_TRANSLATE_NOOP("PlaylistWidget", "Stop after this")},
     .isSeparator = false},
    {.id = PlaybackSeparator, .title = {}, .isSeparator = true},
    {.id          = Remove,
     .title       = {.context = "PlaylistWidget", .sourceText = QT_TRANSLATE_NOOP("PlaylistWidget", "Remove")},
     .isSeparator = false},
    {.id          = Crop,
     .title       = {.context = "PlaylistWidget", .sourceText = QT_TRANSLATE_NOOP("PlaylistWidget", "Crop")},
     .isSeparator = false},
    {.id          = Sort,
     .title       = {.context = "PlaylistWidget", .sourceText = QT_TRANSLATE_NOOP("PlaylistWidget", "Sort")},
     .isSeparator = false},
    {.id = MutateSeparator, .title = {}, .isSeparator = true},
    {.id          = Clipboard,
     .title       = {.context = "PlaylistWidget", .sourceText = QT_TRANSLATE_NOOP("PlaylistWidget", "Clipboard")},
     .isSeparator = false},
    {.id = ClipboardSeparator, .title = {}, .isSeparator = true},
    {.id          = Presets,
     .title       = {.context = "PlaylistWidget", .sourceText = QT_TRANSLATE_NOOP("PlaylistWidget", "Presets")},
     .isSeparator = false},
    {.id = PresetsSeparator, .title = {}, .isSeparator = true},
    {.id    = Constants::Actions::AddToQueue,
     .title = {.context = "PlaylistWidget", .sourceText = QT_TRANSLATE_NOOP("PlaylistWidget", "Add to playback queue")},
     .isSeparator = false},
    {.id    = Constants::Actions::QueueNext,
     .title = {.context = "PlaylistWidget", .sourceText = QT_TRANSLATE_NOOP("PlaylistWidget", "Queue to play next")},
     .isSeparator = false},
    {.id = Constants::Actions::RemoveFromQueue,
     .title
     = {.context = "PlaylistWidget", .sourceText = QT_TRANSLATE_NOOP("PlaylistWidget", "Remove from playback queue")},
     .isSeparator = false},
    {.id          = TrackActions,
     .title       = {.context = "PlaylistWidget", .sourceText = QT_TRANSLATE_NOOP("PlaylistWidget", "Track menu")},
     .isSeparator = false},
});

} // namespace Playlist

namespace LibraryTree {
constexpr auto Play              = "Fooyin.Context.LibraryTree.Play";
constexpr auto PlaybackSeparator = "Fooyin.Context.LibraryTree.Playback.Separator";
constexpr auto Playlist          = "Fooyin.Context.LibraryTree.Playlist";
constexpr auto PlaylistSeparator = "Fooyin.Context.LibraryTree.Playlist.Separator";
constexpr auto Queue             = "Fooyin.Context.LibraryTree.Queue";
constexpr auto QueueSeparator    = "Fooyin.Context.LibraryTree.Queue.Separator";
constexpr auto OpenFolder        = "Fooyin.Context.LibraryTree.OpenFolder";
constexpr auto TrackActions      = "Fooyin.Context.LibraryTree.TrackActions";
constexpr auto WidgetSeparator   = "Fooyin.Context.LibraryTree.Widget.Separator";
constexpr auto Grouping          = "Fooyin.Context.LibraryTree.Grouping";
constexpr auto Configure         = "Fooyin.Context.LibraryTree.Configure";

constexpr auto DefaultItems = std::to_array<Item>({
    {.id          = Play,
     .title       = {.context = "LibraryTreeWidget", .sourceText = QT_TRANSLATE_NOOP("LibraryTreeWidget", "Play")},
     .isSeparator = false},
    {.id = PlaybackSeparator, .title = {}, .isSeparator = true},
    {.id          = Constants::Actions::AddToCurrent,
     .title       = {.context    = "LibraryTreeWidget",
                     .sourceText = QT_TRANSLATE_NOOP("LibraryTreeWidget", "Add to current playlist")},
     .isSeparator = false},
    {.id = Constants::Actions::AddToActive,
     .title
     = {.context = "LibraryTreeWidget", .sourceText = QT_TRANSLATE_NOOP("LibraryTreeWidget", "Add to active playlist")},
     .isSeparator = false},
    {.id          = Constants::Actions::SendToCurrent,
     .title       = {.context    = "LibraryTreeWidget",
                     .sourceText = QT_TRANSLATE_NOOP("LibraryTreeWidget", "Replace current playlist")},
     .isSeparator = false},
    {.id = Constants::Actions::SendToNew,
     .title
     = {.context = "LibraryTreeWidget", .sourceText = QT_TRANSLATE_NOOP("LibraryTreeWidget", "Create new playlist")},
     .isSeparator = false},
    {.id = PlaylistSeparator, .title = {}, .isSeparator = true},
    {.id = Constants::Actions::AddToQueue,
     .title
     = {.context = "LibraryTreeWidget", .sourceText = QT_TRANSLATE_NOOP("LibraryTreeWidget", "Add to playback queue")},
     .isSeparator = false},
    {.id = Constants::Actions::QueueNext,
     .title
     = {.context = "LibraryTreeWidget", .sourceText = QT_TRANSLATE_NOOP("LibraryTreeWidget", "Queue to play next")},
     .isSeparator = false},
    {.id          = Constants::Actions::RemoveFromQueue,
     .title       = {.context    = "LibraryTreeWidget",
                     .sourceText = QT_TRANSLATE_NOOP("LibraryTreeWidget", "Remove from playback queue")},
     .isSeparator = false},
    {.id = QueueSeparator, .title = {}, .isSeparator = true},
    {.id          = Grouping,
     .title       = {.context = "LibraryTreeWidget", .sourceText = QT_TRANSLATE_NOOP("LibraryTreeWidget", "Grouping")},
     .isSeparator = false},
    {.id          = Configure,
     .title       = {.context = "LibraryTreeWidget", .sourceText = QT_TRANSLATE_NOOP("LibraryTreeWidget", "Configure")},
     .isSeparator = false},
    {.id = WidgetSeparator, .title = {}, .isSeparator = true},
    {.id    = OpenFolder,
     .title = {.context = "LibraryTreeWidget", .sourceText = QT_TRANSLATE_NOOP("LibraryTreeWidget", "Open folder")},
     .isSeparator = false},
    {.id    = TrackActions,
     .title = {.context = "LibraryTreeWidget", .sourceText = QT_TRANSLATE_NOOP("LibraryTreeWidget", "Track menu")},
     .isSeparator = false},
});
} // namespace LibraryTree
} // namespace Fooyin::ContextMenuIds
