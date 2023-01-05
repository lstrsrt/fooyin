/*
 * Fooyin
 * Copyright 2022, Luke Taylor <LukeT1@proton.me>
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

#include "musiclibrary.h"

#include "core/app/threadmanager.h"
#include "core/library/models/track.h"
#include "core/library/sorting/sorting.h"
#include "core/playlist/libraryplaylistinterface.h"
#include "librarydatabasemanager.h"
#include "libraryinfo.h"
#include "librarymanager.h"
#include "libraryscanner.h"
#include "musiclibraryinteractor.h"

#include <QTimer>
#include <pluginsystem/pluginmanager.h>
#include <utility>
#include <utils/helpers.h>

namespace Library {
struct MusicLibrary::Private
{
    LibraryPlaylistInterface* playlistInteractor;
    LibraryManager* libraryManager;
    ThreadManager* threadManager;
    LibraryScanner scanner;
    LibraryDatabaseManager libraryDatabaseManager;

    TrackPtrList tracks;
    TrackHash trackMap;

    std::vector<Track*> selectedTracks;
    std::vector<MusicLibraryInteractor*> interactors;

    SortOrder order{Library::SortOrder::YearDesc};

    Private(LibraryPlaylistInterface* playlistInteractor, LibraryManager* libraryManager, ThreadManager* threadManager)
        : playlistInteractor{playlistInteractor}
        , libraryManager{libraryManager}
        , threadManager{threadManager}
        , scanner{libraryManager}
    { }
};

MusicLibrary::MusicLibrary(LibraryPlaylistInterface* playlistInteractor, LibraryManager* libraryManager,
                           ThreadManager* threadManager, QObject* parent)
    : QObject{parent}
    , p{std::make_unique<Private>(playlistInteractor, libraryManager, threadManager)}
{
    p->threadManager->moveToNewThread(&p->scanner);
    p->threadManager->moveToNewThread(&p->libraryDatabaseManager);

    connect(p->libraryManager, &Library::LibraryManager::libraryRemoved, &p->scanner, &LibraryScanner::stopThread);

    connect(this, &MusicLibrary::runLibraryScan, &p->scanner, &LibraryScanner::scanLibrary);
    connect(this, &MusicLibrary::runAllLibrariesScan, &p->scanner, &LibraryScanner::scanAll);
    connect(&p->scanner, &LibraryScanner::libraryAdded, this, &MusicLibrary::libraryAdded);
    connect(&p->scanner, &LibraryScanner::addedTracks, this, &MusicLibrary::newTracksAdded);
    connect(&p->scanner, &LibraryScanner::updatedTracks, this, &MusicLibrary::tracksUpdated);
    connect(&p->scanner, &LibraryScanner::tracksDeleted, this, &MusicLibrary::tracksDeleted);

    connect(&p->libraryDatabaseManager, &LibraryDatabaseManager::gotTracks, this, &MusicLibrary::tracksHaveLoaded);
    connect(this, &MusicLibrary::loadAllTracks, &p->libraryDatabaseManager, &LibraryDatabaseManager::getAllTracks);
    connect(this, &MusicLibrary::updateSaveTracks, &p->libraryDatabaseManager, &LibraryDatabaseManager::updateTracks);

    load();
}

MusicLibrary::~MusicLibrary()
{
    qDeleteAll(p->tracks);
}

void MusicLibrary::load()
{
    getAllTracks();
    QTimer::singleShot(3000, this, &Library::MusicLibrary::reloadAll);
}

void MusicLibrary::libraryAdded()
{
    load();
}

void MusicLibrary::prepareTracks(int idx)
{
    if(p->selectedTracks.empty()) {
        p->playlistInteractor->createPlaylist(tracks(), 0);
    }

    else {
        p->playlistInteractor->createPlaylist(tracks(), idx);
    }
}

TrackPtrList MusicLibrary::selectedTracks()
{
    return p->selectedTracks;
}

void MusicLibrary::updateTracks(const TrackPtrList& tracks)
{
    emit updateSaveTracks(tracks);
}

void MusicLibrary::addInteractor(MusicLibraryInteractor* interactor)
{
    p->interactors.emplace_back(interactor);
}

void MusicLibrary::tracksHaveLoaded(const TrackList& tracks)
{
    qDeleteAll(p->tracks);
    refreshTracks(tracks);
}

void MusicLibrary::newTracksAdded(const TrackList& tracks)
{
    for(const auto& track : tracks) {
        auto* trackPtr = new Track(track);
        p->tracks.emplace_back(trackPtr);
        p->trackMap.emplace(trackPtr->id(), trackPtr);
    }
    Library::sortTracks(p->tracks, p->order);
}

void MusicLibrary::tracksUpdated(const TrackList& tracks)
{
    for(const auto& track : tracks) {
        if(p->trackMap.count(track.id())) {
            Track* libraryTrack = p->trackMap.at(track.id());
            if(libraryTrack) {
                *libraryTrack = track;
            }
        }
    }
}

void MusicLibrary::tracksDeleted(const IdSet& tracks)
{
    for(auto trackId : tracks) {
        if(p->trackMap.count(trackId)) {
            {
                Track* libraryTrack = p->trackMap.at(trackId);
                libraryTrack->setIsEnabled(false);
            }
        }
    }
}

void MusicLibrary::reloadAll()
{
    emit runAllLibrariesScan(p->tracks);
}

void MusicLibrary::reload(const Library::LibraryInfo& info)
{
    emit runLibraryScan(p->tracks, info);
}

void MusicLibrary::refresh()
{
    getAllTracks();
}

void MusicLibrary::refreshTracks(const TrackList& result)
{
    p->tracks.clear();
    p->trackMap.clear();
    for(const auto& track : result) {
        auto* trackPtr = new Track(track);
        p->tracks.emplace_back(trackPtr);
        p->trackMap.emplace(trackPtr->id(), trackPtr);
    }
    Library::sortTracks(p->tracks, p->order);
    emit tracksLoaded(p->tracks);
}

TrackPtrList MusicLibrary::tracks()
{
    TrackPtrList lst;
    bool haveTracks{false};
    for(auto* inter : p->interactors) {
        if(inter->hasTracks()) {
            haveTracks = true;
            auto trks = inter->tracks();
            lst.insert(lst.end(), trks.begin(), trks.end());
        }
    }
    return !haveTracks ? p->tracks : lst;
}

TrackPtrList MusicLibrary::allTracks()
{
    return p->tracks;
}

Library::SortOrder MusicLibrary::sortOrder()
{
    return p->order;
}

void MusicLibrary::changeOrder(SortOrder order)
{
    p->order = order;
    Library::sortTracks(p->tracks, p->order);
    //    if(!p->filteredTracks.empty()) {
    //        Library::sortTracks(p->filteredTracks, p->order);
    //    }
}

void MusicLibrary::changeTrackSelection(const QSet<Track*>& tracks)
{
    std::vector<Track*> newSelectedTracks;
    for(const auto& track : tracks) {
        newSelectedTracks.emplace_back(track);
    }

    if(p->selectedTracks == newSelectedTracks) {
        return;
    }

    p->selectedTracks = std::move(newSelectedTracks);
}

void MusicLibrary::trackSelectionChanged(const QSet<Track*>& tracks)
{
    if(tracks.isEmpty()) {
        return;
    }

    changeTrackSelection(tracks);
    emit tracksSelChanged();
}

void MusicLibrary::getAllTracks()
{
    emit loadAllTracks();
}
} // namespace Library
