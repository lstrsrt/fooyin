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
#include "internalguisettings.h"

#include <gui/guiconstants.h>
#include <utils/settings/settingsmanager.h>

namespace Fooyin {
PlaylistContextMenuPage::PlaylistContextMenuPage(SettingsManager* settings, QObject* parent)
    : SettingsPage{settings->settingsDialog(), parent}
{
    setId(Constants::Page::InterfaceContextMenuPlaylistWidget);
    setName(tr("Playlist Widget"));
    setCategory({tr("Interface"), tr("Context Menu")});
    setWidgetCreator([settings] {
        return new ConfigurableContextMenuWidget(
            tr("Unchecked items will be hidden from the playlist widget context menu."),
            [settings] { return settings->value<Settings::Gui::Internal::PlaylistContextMenuDisabledSections>(); },
            [settings](const QStringList&, const QStringList& disabledIds) {
                settings->set<Settings::Gui::Internal::PlaylistContextMenuDisabledSections>(disabledIds);
            },
            {{.title       = {},
              .nodeFactory = [] { return playlistWidgetContextMenuNodes(); },
              .row         = 0,
              .column      = 0,
              .rowSpan     = 1,
              .columnSpan  = 1}});
    });
}

ContextMenuNodeList PlaylistContextMenuPage::playlistWidgetContextMenuNodes()
{
    const auto node = [](const char* id, const QString& title, const char* parentId = nullptr) {
        return ConfigurableContextMenuNode{
            .id       = QString::fromUtf8(id),
            .title    = title,
            .parentId = parentId ? QString::fromUtf8(parentId) : QString{},
        };
    };

    return {
        node(Constants::Menus::Context::PlaylistWidget::Play, PlaylistContextMenuPage::tr("Play")),
        node(Constants::Menus::Context::PlaylistWidget::StopAfterThis, PlaylistContextMenuPage::tr("Stop after this")),
        node(Constants::Menus::Context::PlaylistWidget::Remove, PlaylistContextMenuPage::tr("Remove")),
        node(Constants::Menus::Context::PlaylistWidget::Crop, PlaylistContextMenuPage::tr("Crop")),
        node(Constants::Menus::Context::PlaylistWidget::Sort, PlaylistContextMenuPage::tr("Sort")),
        node(Constants::Menus::Context::PlaylistWidget::Clipboard, PlaylistContextMenuPage::tr("Clipboard")),
        node(Constants::Menus::Context::PlaylistWidget::Presets, PlaylistContextMenuPage::tr("Presets")),
        node(Constants::Menus::Context::PlaylistWidget::Queue, PlaylistContextMenuPage::tr("Queue")),
        node(Constants::Menus::Context::PlaylistWidget::TrackActions, PlaylistContextMenuPage::tr("Track actions")),
    };
}
} // namespace Fooyin
