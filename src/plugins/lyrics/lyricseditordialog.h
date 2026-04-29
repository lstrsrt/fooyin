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

#include <core/track.h>

#include <QDialog>

namespace Fooyin {
class PlayerController;

namespace Lyrics {
class LyricsEditor;
class LyricsSaver;

class LyricsEditorDialog : public QDialog
{
    Q_OBJECT

public:
    LyricsEditorDialog(const Track& track, const Lyrics& lyrics, PlayerController* playerController,
                       LyricsSaver* lyricsSaver, QWidget* parent = nullptr);

    [[nodiscard]] LyricsEditor* editor() const;

    void saveState();
    void restoreState();

    void accept() override;
    [[nodiscard]] QSize sizeHint() const override;

Q_SIGNALS:
    void lyricsEdited(const Fooyin::Lyrics::Lyrics& lyrics);

private:
    [[nodiscard]] bool apply();

    Track m_track;

    LyricsSaver* m_lyricsSaver;
    LyricsEditor* m_editor;
};
} // namespace Lyrics
} // namespace Fooyin
