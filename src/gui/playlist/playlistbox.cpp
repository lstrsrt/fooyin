/*
 * Fooyin
 * Copyright © 2024, Luke Taylor <LukeT1@proton.me>
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

#include "playlistbox.h"

#include "dialog/autoplaylistdialog.h"
#include "playlist/playlistcontroller.h"

#include <core/playlist/playlisthandler.h>
#include <gui/widgets/popuplineedit.h>
#include <utils/utils.h>

#include <QAction>
#include <QComboBox>
#include <QMainWindow>
#include <QMenu>
#include <QSignalBlocker>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace Fooyin {
PlaylistBox::PlaylistBox(PlaylistController* playlistController, QWidget* parent)
    : FyWidget{parent}
    , m_playlistController{playlistController}
    , m_playlistHandler{m_playlistController->playlistHandler()}
    , m_playlistBox{new QComboBox(this)}
    , m_lineEdit{nullptr}
    , m_renameCancelled{false}
{
    QObject::setObjectName(PlaylistBox::name());

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});

    layout->addWidget(m_playlistBox);

    QObject::connect(m_playlistBox, &QComboBox::currentIndexChanged, this, &PlaylistBox::changePlaylist);
    m_playlistBox->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(m_playlistBox, &QWidget::customContextMenuRequested, this, &PlaylistBox::showContextMenu);
    QObject::connect(m_playlistController, &PlaylistController::playlistsLoaded, this,
                     [this]() { currentPlaylistChanged(m_playlistController->currentPlaylist()); });
    QObject::connect(
        m_playlistController, &PlaylistController::currentPlaylistChanged, this,
        [this](const Playlist* /*prevPlaylist*/, const Playlist* playlist) { currentPlaylistChanged(playlist); });
    QObject::connect(m_playlistHandler, &PlaylistHandler::playlistAdded, this, &PlaylistBox::addPlaylist);
    QObject::connect(m_playlistHandler, &PlaylistHandler::playlistRemoved, this, &PlaylistBox::removePlaylist);
    QObject::connect(m_playlistHandler, &PlaylistHandler::playlistRenamed, this,
                     [this](const Playlist* playlist) { playlistRenamed(playlist); });

    setupBox();
    currentPlaylistChanged(m_playlistController->currentPlaylist());
}

QString PlaylistBox::name() const
{
    return tr("Playlist Switcher");
}

QString PlaylistBox::layoutName() const
{
    return u"PlaylistSwitcher"_s;
}

void PlaylistBox::setupBox()
{
    // Prevent currentIndexChanged triggering changePlaylist
    const QSignalBlocker block{m_playlistBox};

    const auto& playlists = m_playlistHandler->playlists();
    for(const auto& playlist : playlists) {
        addPlaylist(playlist);
    }
}

void PlaylistBox::addPlaylist(const Playlist* playlist)
{
    if(!playlist) {
        return;
    }

    m_playlistBox->addItem(playlist->name(), QVariant::fromValue(playlist->id()));
}

void PlaylistBox::removePlaylist(const Playlist* playlist)
{
    if(!playlist) {
        return;
    }

    if(const auto* current = currentPlaylist(); current && current->id() == playlist->id()) {
        closeRenameEditor();
    }

    const int count = m_playlistBox->count();

    for(int i{0}; i < count; ++i) {
        if(m_playlistBox->itemData(i).value<UId>() == playlist->id()) {
            m_playlistBox->removeItem(i);
        }
    }
}

void PlaylistBox::playlistRenamed(const Playlist* playlist) const
{
    if(!playlist) {
        return;
    }

    const int count = m_playlistBox->count();

    for(int i{0}; i < count; ++i) {
        if(m_playlistBox->itemData(i).value<UId>() == playlist->id()) {
            m_playlistBox->setItemText(i, playlist->name());
        }
    }
}

void PlaylistBox::currentPlaylistChanged(const Playlist* playlist) const
{
    if(!playlist) {
        return;
    }

    const int count = m_playlistBox->count();
    const UId id    = playlist->id();

    for(int i{0}; i < count; ++i) {
        if(m_playlistBox->itemData(i).value<UId>() == id) {
            // Prevent currentIndexChanged triggering changePlaylist
            const QSignalBlocker block{m_playlistBox};
            m_playlistBox->setCurrentIndex(i);
        }
    }
}

void PlaylistBox::changePlaylist(int index)
{
    if(index < 0 || index >= m_playlistBox->count()) {
        return;
    }

    const auto id = m_playlistBox->itemData(index).value<UId>();
    m_playlistController->changeCurrentPlaylist(id);
}

void PlaylistBox::showContextMenu(const QPoint& pos)
{
    auto* menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    auto* createPlaylist = new QAction(tr("Add &new playlist"), menu);
    QObject::connect(createPlaylist, &QAction::triggered, this, [this]() {
        closeRenameEditor();

        if(auto* playlist = m_playlistHandler->createEmptyPlaylist()) {
            m_playlistController->changeCurrentPlaylist(playlist);
        }
    });
    menu->addAction(createPlaylist);

    auto* createAutoPlaylist = new QAction(tr("Add new &autoplaylist"), menu);
    QObject::connect(createAutoPlaylist, &QAction::triggered, this, [this]() {
        closeRenameEditor();

        auto* autoDialog = new AutoPlaylistDialog(Utils::getMainWindow());
        autoDialog->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(autoDialog, &AutoPlaylistDialog::playlistEdited, autoDialog,
                         [this](const QString& name, const QString& query, const QString& sortQuery, bool forceSorted) {
                             if(auto* playlist
                                = m_playlistHandler->createNewAutoPlaylist(name, query, sortQuery, forceSorted)) {
                                 m_playlistController->changeCurrentPlaylist(playlist);
                             }
                         });
        autoDialog->show();
    });
    menu->addAction(createAutoPlaylist);

    if(auto* playlist = currentPlaylist()) {
        menu->addSeparator();

        if(playlist->isAutoPlaylist()) {
            auto* editAutoPlaylist = new QAction(tr("&Edit autoplaylist"), menu);
            QObject::connect(editAutoPlaylist, &QAction::triggered, this, [this, playlist]() {
                closeRenameEditor();

                auto* autoDialog = new AutoPlaylistDialog(m_playlistHandler, playlist, Utils::getMainWindow());
                autoDialog->setAttribute(Qt::WA_DeleteOnClose);
                autoDialog->show();
            });
            menu->addAction(editAutoPlaylist);
        }

        auto* renameAction
            = new QAction(playlist->isAutoPlaylist() ? tr("Re&name autoplaylist") : tr("Re&name playlist"), menu);
        QObject::connect(renameAction, &QAction::triggered, this, &PlaylistBox::showRenameEditor);
        menu->addAction(renameAction);

        auto* removeAction
            = new QAction(playlist->isAutoPlaylist() ? tr("&Remove autoplaylist") : tr("&Remove playlist"), menu);
        QObject::connect(removeAction, &QAction::triggered, this, [this, playlist]() {
            closeRenameEditor();
            m_playlistHandler->removePlaylist(playlist->id());
        });
        menu->addAction(removeAction);
    }

    menu->popup(m_playlistBox->mapToGlobal(pos));
}

void PlaylistBox::showRenameEditor()
{
    auto* playlist = currentPlaylist();
    if(!playlist) {
        return;
    }

    closeRenameEditor();
    m_renameCancelled = false;

    m_lineEdit = new PopupLineEdit(playlist->name(), this);
    m_lineEdit->setAttribute(Qt::WA_DeleteOnClose);
    m_lineEdit->setGeometry(m_playlistBox->geometry());

    QObject::connect(m_lineEdit, &PopupLineEdit::editingCancelled, this, &PlaylistBox::cancelRenameEditor);
    QObject::connect(m_lineEdit, &QLineEdit::editingFinished, this, [this, playlistId = playlist->id()]() {
        if(!m_lineEdit) {
            return;
        }

        const QString text = m_lineEdit->text();
        closeRenameEditor();

        if(m_renameCancelled) {
            return;
        }

        if(auto* current = m_playlistHandler->playlistById(playlistId)) {
            if(text != current->name()) {
                m_playlistHandler->renamePlaylist(playlistId, text);
            }
        }
    });

    m_lineEdit->show();
    m_lineEdit->selectAll();
    m_lineEdit->setFocus(Qt::ActiveWindowFocusReason);
}

void PlaylistBox::closeRenameEditor()
{
    if(m_lineEdit) {
        m_lineEdit->close();
    }
}

void PlaylistBox::cancelRenameEditor()
{
    m_renameCancelled = true;
    closeRenameEditor();
}

Playlist* PlaylistBox::currentPlaylist() const
{
    return m_playlistController->currentPlaylist();
}
} // namespace Fooyin
