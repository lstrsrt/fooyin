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

#include "lyrics.h"

#include <QAbstractTableModel>

namespace Fooyin::Lyrics {
class LyricsSearchModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit LyricsSearchModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void clear();
    void addLyrics(const Lyrics& lyrics);
    void updateLyrics(int row, const Lyrics& lyrics);

    [[nodiscard]] const Lyrics* lyricsAt(int row) const;
    [[nodiscard]] int resultCount() const;

private:
    static QString lyricsTypeText(const Fooyin::Lyrics::Lyrics& lyrics);

    std::vector<Lyrics> m_results;
};
} // namespace Fooyin::Lyrics
