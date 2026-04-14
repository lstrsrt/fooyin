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

#include "playlistcontextmenupage.h"

#include "configurablecontextmenupage.h"
#include "contextmenuids.h"
#include "internalguisettings.h"

#include <gui/guiconstants.h>
#include <utils/settings/settingsmanager.h>

namespace Fooyin {
PlaylistContextMenuPage::PlaylistContextMenuPage(SettingsManager* settings, QObject* parent)
    : SettingsPage{settings->settingsDialog(), parent}
{
    setId(Constants::Page::InterfaceContextMenuPlaylistWidget);
    setName(tr("Playlist"));
    setCategory({tr("Interface"), tr("Context Menu")});
    setWidgetCreator([settings] {
        return new ConfigurableContextMenuWidget(
            tr("Unchecked items will be hidden from the playlist context menu."),
            {.nodeFactory = [] { return playlistWidgetContextMenuNodes(); },
             .readDisabledIds
             = ContextMenuSettings::makeStringListReader<Settings::Gui::Internal::ContextMenuPlaylistDisabledSections>(
                 settings),
             .writeDisabledIds
             = ContextMenuSettings::makeStringListWriter<Settings::Gui::Internal::ContextMenuPlaylistDisabledSections>(
                 settings),
             .readTopLevelOrder
             = ContextMenuSettings::makeStringListReader<Settings::Gui::Internal::ContextMenuPlaylistLayout>(settings),
             .writeTopLevelOrder
             = ContextMenuSettings::makeStringListWriter<Settings::Gui::Internal::ContextMenuPlaylistLayout>(settings),
             .allowReordering = true});
    });
}

ContextMenuNodeList PlaylistContextMenuPage::playlistWidgetContextMenuNodes()
{
    ContextMenuNodeList nodes;
    nodes.reserve(ContextMenuIds::Playlist::DefaultItems.size());

    for(const auto& item : ContextMenuIds::Playlist::DefaultItems) {
        nodes.emplace_back(QString::fromUtf8(item.id), item.isSeparator ? QString{} : QString::fromUtf8(item.title),
                           QString{}, item.isSeparator);
    }

    return nodes;
}
} // namespace Fooyin
