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

#include "trackcontextmenupage.h"

#include "configurablecontextmenupage.h"
#include "internalguisettings.h"

#include <gui/guiconstants.h>
#include <utils/settings/settingsmanager.h>

using namespace Qt::StringLiterals;

namespace Fooyin {
TrackContextMenuPage::TrackContextMenuPage(TrackSelectionController* trackSelection, SettingsManager* settings,
                                           TrackContextMenuPageMode mode, QObject* parent)
    : SettingsPage{settings->settingsDialog(), parent}
{
    switch(mode) {
        case TrackContextMenuPageMode::PlaylistQueue:
            setId(Constants::Page::InterfaceContextMenuPlaylist);
            setName(tr("Playlist / Queue"));
            break;
        case TrackContextMenuPageMode::Track:
            setId(Constants::Page::InterfaceContextMenuTrack);
            setName(tr("Track"));
            break;
    }

    setCategory({tr("Interface"), tr("Context Menu")});
    setWidgetCreator([trackSelection, settings, mode] {
        ContextMenuSectionList sections;

        if(mode == TrackContextMenuPageMode::Track) {
            sections.push_back({
                .title = {},
                .nodeFactory
                = [trackSelection] { return trackContextMenuNodes(trackSelection, TrackContextMenuArea::Track); },
                .row        = 0,
                .column     = 0,
                .rowSpan    = 1,
                .columnSpan = 1,
            });
        }
        else {
            sections.push_back({
                .title = tr("Playlist actions"),
                .nodeFactory
                = [trackSelection] { return trackContextMenuNodes(trackSelection, TrackContextMenuArea::Playlist); },
                .row    = 0,
                .column = 0,
            });
            sections.push_back({
                .title = tr("Queue actions"),
                .nodeFactory
                = [trackSelection] { return trackContextMenuNodes(trackSelection, TrackContextMenuArea::Queue); },
                .row    = 0,
                .column = 1,
            });
        }

        return new ConfigurableContextMenuWidget(
            tr("Unchecked items will be hidden from track selection context menus."),
            [settings] { return settings->value<Settings::Gui::Internal::TrackContextMenuDisabledSections>(); },
            [settings](const QStringList& allNodeIds, const QStringList& disabledIds) {
                QStringList mergedDisabledIds
                    = settings->value<Settings::Gui::Internal::TrackContextMenuDisabledSections>();

                for(const auto& id : allNodeIds) {
                    mergedDisabledIds.removeAll(id);
                }

                mergedDisabledIds.append(disabledIds);
                mergedDisabledIds.removeDuplicates();

                settings->set<Settings::Gui::Internal::TrackContextMenuDisabledSections>(mergedDisabledIds);
            },
            sections);
    });
}

ContextMenuNodeList TrackContextMenuPage::trackContextMenuNodes(TrackSelectionController* trackSelection,
                                                                TrackContextMenuArea area)
{
    ContextMenuNodeList nodes;

    if(!trackSelection) {
        return nodes;
    }

    const auto menuNodes = trackSelection->trackContextMenuNodes();
    for(const auto& node : menuNodes) {
        if(node.area != area || !node.parentId) {
            continue;
        }

        nodes.push_back({
            .id       = node.id.name(),
            .title    = node.title,
            .parentId = node.parentId ? node.parentId->name() : QString{},
        });
    }

    return nodes;
}
} // namespace Fooyin
