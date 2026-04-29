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

#include "lyricseditordialog.h"

#include "lyricseditor.h"
#include "lyricssaver.h"

#include <utils/utils.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace Fooyin::Lyrics {
LyricsEditorDialog::LyricsEditorDialog(const Track& track, const Lyrics& lyrics, PlayerController* playerController,
                                       LyricsSaver* lyricsSaver, QWidget* parent)
    : QDialog{parent}
    , m_track{track}
    , m_lyricsSaver{lyricsSaver}
    , m_editor{new LyricsEditor(playerController, this)}
{
    setWindowTitle(tr("Lyrics Editor"));

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_editor);

    auto* buttonBox
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);

    QObject::connect(buttonBox->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this,
                     &LyricsEditorDialog::apply);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(buttonBox);

    if(m_lyricsSaver) {
        QObject::connect(m_lyricsSaver, &LyricsSaver::lyricsSaved, this, [this](const Track& updatedTrack) {
            if(updatedTrack.sameIdentityAs(m_track)) {
                m_track = updatedTrack;
            }
        });
    }

    m_editor->setTrack(track);
    m_editor->setLyrics(lyrics);
}

LyricsEditor* LyricsEditorDialog::editor() const
{
    return m_editor;
}

void LyricsEditorDialog::saveState()
{
    FyStateSettings stateSettings;
    Utils::saveState(this, stateSettings);
}

void LyricsEditorDialog::restoreState()
{
    const FyStateSettings stateSettings;
    Utils::restoreState(this, stateSettings);
}

void LyricsEditorDialog::accept()
{
    if(apply()) {
        QDialog::accept();
    }
}

QSize LyricsEditorDialog::sizeHint() const
{
    return m_editor->sizeHint();
}

bool LyricsEditorDialog::apply()
{
    const Lyrics updatedLyrics = m_editor->editedLyrics();

    if(m_lyricsSaver && m_track.isValid() && !m_lyricsSaver->saveLyrics(updatedLyrics, m_track)) {
        return false;
    }

    Lyrics savedLyrics{updatedLyrics};
    if(m_lyricsSaver && m_track.isValid()) {
        savedLyrics = m_lyricsSaver->savedLyrics(updatedLyrics, m_track);
    }
    else {
        savedLyrics.isLocal = true;
    }

    Q_EMIT lyricsEdited(savedLyrics);
    m_editor->setLyrics(savedLyrics);
    return true;
}
} // namespace Fooyin::Lyrics
