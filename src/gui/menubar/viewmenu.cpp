/*
 * Fooyin
 * Copyright © 2023, Luke Taylor <LukeT1@proton.me>
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

#include "viewmenu.h"

#include <gui/guiconstants.h>
#include <gui/iconloader.h>
#include <utils/actions/actioncontainer.h>
#include <utils/actions/actionmanager.h>
#include <utils/actions/command.h>
#include <utils/settings/settingsmanager.h>
#include <utils/utils.h>

#include <QAction>

namespace Fooyin {
ViewMenu::ViewMenu(ActionManager* actionManager, SettingsManager* settings, QObject* parent)
    : QObject{parent}
    , m_actionManager{actionManager}
    , m_settings{settings}
{
    auto* viewMenu = m_actionManager->actionContainer(Constants::Menus::View);

    const QStringList viewCategory = {tr("View")};

    auto* openQuickSetup = new QAction(tr("&Quick setup"), this);
    Gui::setThemeIcon(openQuickSetup, Constants::Icons::QuickSetup);
    openQuickSetup->setStatusTip(tr("Open the quick setup dialog"));
    auto* quickSetupCmd = m_actionManager->registerAction(openQuickSetup, Constants::Actions::QuickSetup);
    quickSetupCmd->setCategories(viewCategory);
    viewMenu->addAction(quickSetupCmd, Actions::Groups::One);
    QObject::connect(openQuickSetup, &QAction::triggered, this, &ViewMenu::openQuickSetup);

    auto* showLog = new QAction(tr("&Log"), this);
    Gui::setThemeIcon(showLog, Constants::Icons::Log);
    showLog->setStatusTip(tr("Open the log dialog"));
    auto* showLogCmd = m_actionManager->registerAction(showLog, Constants::Actions::Log);
    showLogCmd->setCategories(viewCategory);
    viewMenu->addAction(showLogCmd);
    QObject::connect(showLog, &QAction::triggered, this, &ViewMenu::openLog);

    auto* showEditor = new QAction(tr("&Script editor"), this);
    Gui::setThemeIcon(showEditor, Constants::Icons::ScriptEditor);
    showEditor->setStatusTip(tr("Open the script editor dialog"));
    auto* showEditorCmd = m_actionManager->registerAction(showEditor, Constants::Actions::ScriptEditor);
    showEditorCmd->setCategories(viewCategory);
    viewMenu->addAction(showEditorCmd);
    QObject::connect(showEditor, &QAction::triggered, this, &ViewMenu::openScriptEditor);

    auto* showPlaylistManager = new QAction(tr("&Playlist Manager"), this);
    showPlaylistManager->setStatusTip(tr("Open the playlist manager window"));
    auto* showPlaylistManagerCmd
        = m_actionManager->registerAction(showPlaylistManager, Constants::Actions::PlaylistManager);
    showPlaylistManagerCmd->setCategories(viewCategory);
    viewMenu->addAction(showPlaylistManagerCmd);
    QObject::connect(showPlaylistManager, &QAction::triggered, this, &ViewMenu::openPlaylistManager);

    viewMenu->addSeparator();

    auto* focusSearchBar = new QAction(tr("Focus Search &Bar"), this);
    focusSearchBar->setStatusTip(tr("Focus the first Search Bar found in the current layout"));
    auto* focusSearchBarCmd = m_actionManager->registerAction(focusSearchBar, Constants::Actions::FocusSearchBar);
    focusSearchBarCmd->setCategories(viewCategory);
    viewMenu->addAction(focusSearchBarCmd);
    QObject::connect(focusSearchBar, &QAction::triggered, this, &ViewMenu::focusSearchBar);

    auto* showNowPlaying = new QAction(tr("Show playing &track"), this);
    showNowPlaying->setStatusTip(tr("Show the currently playing track in the playlist"));
    auto* showNowPlayingCmd = m_actionManager->registerAction(showNowPlaying, Constants::Actions::ShowNowPlaying);
    showNowPlayingCmd->setCategories(viewCategory);
    viewMenu->addAction(showNowPlayingCmd);
    QObject::connect(showNowPlaying, &QAction::triggered, this, &ViewMenu::showNowPlaying);
}
} // namespace Fooyin

#include "moc_viewmenu.cpp"
