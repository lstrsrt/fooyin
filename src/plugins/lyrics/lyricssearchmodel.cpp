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

#include "lyricssearchmodel.h"

using namespace Qt::StringLiterals;

namespace Fooyin::Lyrics {
LyricsSearchModel::LyricsSearchModel(QObject* parent)
    : QAbstractTableModel{parent}
{ }

int LyricsSearchModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_results.size());
}

int LyricsSearchModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 5;
}

QVariant LyricsSearchModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const Lyrics& lyrics = m_results.at(index.row());

    if(role == Qt::DisplayRole) {
        switch(index.column()) {
            case 0:
                return lyrics.metadata.title;
            case 1:
                return lyrics.metadata.album;
            case 2:
                return lyrics.metadata.artist;
            case 3:
                return lyrics.source;
            case 4:
                return lyricsTypeText(lyrics);
            default:
                return {};
        }
    }

    if(role == Qt::TextAlignmentRole && index.column() == 4) {
        return static_cast<int>(Qt::AlignCenter);
    }

    return {};
}

QVariant LyricsSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation != Qt::Horizontal) {
        return {};
    }

    if(role == Qt::TextAlignmentRole) {
        return Qt::AlignCenter;
    }

    if(role != Qt::DisplayRole) {
        return {};
    }

    switch(section) {
        case 0:
            return tr("Title");
        case 1:
            return tr("Album");
        case 2:
            return tr("Artist");
        case 3:
            return tr("Source");
        case 4:
            return tr("Synced");
        default:
            return {};
    }
}

void LyricsSearchModel::clear()
{
    beginResetModel();
    m_results.clear();
    endResetModel();
}

void LyricsSearchModel::addLyrics(const Lyrics& lyrics)
{
    const int row = rowCount();
    beginInsertRows({}, row, row);
    m_results.emplace_back(lyrics);
    endInsertRows();
}

void LyricsSearchModel::updateLyrics(int row, const Lyrics& lyrics)
{
    if(row < 0 || row >= rowCount()) {
        return;
    }

    m_results[static_cast<size_t>(row)] = lyrics;
    const QModelIndex first             = index(row, 0);
    const QModelIndex last              = index(row, columnCount() - 1);
    emit dataChanged(first, last);
}

const Lyrics* LyricsSearchModel::lyricsAt(int row) const
{
    if(row < 0 || row >= rowCount()) {
        return nullptr;
    }

    return &m_results.at(row);
}

int LyricsSearchModel::resultCount() const
{
    return rowCount();
}

QString LyricsSearchModel::lyricsTypeText(const Lyrics& lyrics)
{
    switch(lyrics.type) {
        case Lyrics::Type::Unsynced:
            return tr("No");
        case Lyrics::Type::Synced:
            return tr("Line");
        case Lyrics::Type::SyncedWords:
            return tr("Word");
        case Lyrics::Type::Unknown:
            break;
    }

    return QString{};
}
} // namespace Fooyin::Lyrics
