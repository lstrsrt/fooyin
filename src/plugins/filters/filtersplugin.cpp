/*
 * Fooyin
 * Copyright 2022-2023, Luke Taylor <LukeT1@proton.me>
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

#include "filtersplugin.h"

#include "filtermanager.h"
#include "filtersettings.h"
#include "filterwidget.h"
#include "searchwidget.h"

#include <core/actions/actioncontainer.h>
#include <gui/widgetfactory.h>

#include <QMenu>

namespace Filters {
void FiltersPlugin::initialise()
{
    m_filterManager  = new FilterManager(m_threadManager, m_database, m_library, this);
    m_filterSettings = std::make_unique<Settings::FiltersSettings>(m_settings);

    registerFilter<FilterWidget>("Filter", "Filter");
    registerFilter<GenreFilter>("FilterGenre", "Genre");
    registerFilter<YearFilter>("FilterYear", "Year");
    registerFilter<AlbumArtistFilter>("FilterAlbumArtist", "Album Artist");
    registerFilter<ArtistFilter>("FilterArtist", "Artist");
    registerFilter<AlbumFilter>("FilterAlbum", "Album");

    m_factory->registerClass<SearchWidget>("Search", [this]() {
        return new SearchWidget(m_filterManager, m_settings);
    });
}

void FiltersPlugin::initialise(WidgetPluginContext context)
{
    m_actionManager = context.actionManager;
    m_library       = context.library;
    m_playerManager = context.playerManager;
    m_factory       = context.widgetFactory;
}

void FiltersPlugin::initialise(SettingsPluginContext context)
{
    m_settings = context.settingsManager;
}

void FiltersPlugin::initialise(ThreadPluginContext context)
{
    m_threadManager = context.threadManager;
}

void FiltersPlugin::initialise(DatabasePluginContext context)
{
    m_database = context.database;
}
} // namespace Filters
