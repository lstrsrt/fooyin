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

#include "playlistsavingpage.h"

#include "internalguisettings.h"

#include <core/corepaths.h>
#include <core/coresettings.h>
#include <core/internalcoresettings.h>
#include <gui/guiconstants.h>
#include <gui/iconloader.h>
#include <utils/settings/settingsmanager.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>

using namespace Qt::StringLiterals;

namespace Fooyin {
class PlaylistSavingPageWidget : public SettingsPageWidget
{
    Q_OBJECT

public:
    explicit PlaylistSavingPageWidget(const QStringList& playlistExtensions, SettingsManager* settings);

    void load() override;
    void apply() override;
    void reset() override;

private:
    void browseExportPath();

    QStringList m_playlistExtensions;
    SettingsManager* m_settings;

    QComboBox* m_exportPathType;
    QCheckBox* m_exportMetadata;
    QGroupBox* m_autoExporting;
    QComboBox* m_autoExportType;
    QLineEdit* m_autoExportPath;
    QCheckBox* m_autoExportRemove;
    QCheckBox* m_autoExportSaveRemoved;
};

PlaylistSavingPageWidget::PlaylistSavingPageWidget(const QStringList& playlistExtensions, SettingsManager* settings)
    : m_playlistExtensions{playlistExtensions}
    , m_settings{settings}
    , m_exportPathType{new QComboBox(this)}
    , m_exportMetadata{new QCheckBox(tr("Write metadata"), this)}
    , m_autoExporting{new QGroupBox(tr("Auto-export"), this)}
    , m_autoExportType{new QComboBox(this)}
    , m_autoExportPath{new QLineEdit(this)}
    , m_autoExportRemove{new QCheckBox(tr("Move deleted or empty playlists to trash"), this)}
    , m_autoExportSaveRemoved{new QCheckBox(tr("Save last state of deleted playlists"), this)}
{
    const auto saveRemovedTooltip = tr("Save any changes made to a playlist before it was deleted");
    m_autoExportSaveRemoved->setToolTip(saveRemovedTooltip);
    QObject::connect(m_autoExportRemove, &QCheckBox::clicked, m_autoExportSaveRemoved,
                     [this](bool checked) { m_autoExportSaveRemoved->setEnabled(!checked); });

    auto* saving       = new QGroupBox(tr("Saving"), this);
    auto* savingLayout = new QGridLayout(saving);

    int row{0};
    savingLayout->addWidget(new QLabel(tr("Path type") + ":"_L1, this), row, 0);
    savingLayout->addWidget(m_exportPathType, row++, 1);
    savingLayout->addWidget(m_exportMetadata, row++, 0, 1, 2);
    savingLayout->setColumnStretch(2, 1);

    m_autoExporting->setToolTip(tr("Export and synchronise playlists in the specified format and location"));

    auto* browseAction = new QAction(this);
    Gui::setThemeIcon(browseAction, Constants::Icons::Options);
    QObject::connect(browseAction, &QAction::triggered, this, &PlaylistSavingPageWidget::browseExportPath);
    m_autoExportPath->addAction(browseAction, QLineEdit::TrailingPosition);

    auto* autoExportLayout = new QGridLayout(m_autoExporting);
    m_autoExporting->setCheckable(true);

    row = 0;
    autoExportLayout->addWidget(new QLabel(tr("Format") + ":"_L1, this), row, 0);
    autoExportLayout->addWidget(m_autoExportType, row++, 1);
    autoExportLayout->addWidget(new QLabel(tr("Location") + ":"_L1, this), row, 0);
    autoExportLayout->addWidget(m_autoExportPath, row++, 1, 1, 2);
    autoExportLayout->addWidget(m_autoExportRemove, row++, 0, 1, 3);
    autoExportLayout->addWidget(m_autoExportSaveRemoved, row++, 0, 1, 3);
    autoExportLayout->setColumnStretch(2, 1);

    auto* mainLayout = new QGridLayout(this);

    row = 0;
    mainLayout->addWidget(saving, row++, 0);
    mainLayout->addWidget(m_autoExporting, row++, 0);
    mainLayout->setRowStretch(mainLayout->rowCount(), 1);

    m_exportPathType->addItem(tr("Auto"));
    m_exportPathType->addItem(tr("Absolute"));
    m_exportPathType->addItem(tr("Relative"));
}

void PlaylistSavingPageWidget::load()
{
    m_exportPathType->setCurrentIndex(m_settings->fileValue(Settings::Core::Internal::PlaylistSavePathType, 0).toInt());
    m_exportMetadata->setChecked(m_settings->fileValue(Settings::Core::Internal::PlaylistSaveMetadata, false).toBool());

    m_autoExportType->clear();
    for(const QString& ext : std::as_const(m_playlistExtensions)) {
        m_autoExportType->addItem(ext);
    }

    m_autoExporting->setChecked(m_settings->fileValue(Settings::Core::Internal::AutoExportPlaylists, false).toBool());
    m_autoExportType->setCurrentText(
        m_settings->fileValue(Settings::Core::Internal::AutoExportPlaylistsType, u"m3u8"_s).toString());
    m_autoExportPath->setText(
        m_settings->fileValue(Settings::Core::Internal::AutoExportPlaylistsPath, Core::playlistsPath()).toString());
    m_autoExportRemove->setChecked(
        m_settings->fileValue(Settings::Core::Internal::AutoExportPlaylistsRemove, true).toBool());
    m_autoExportSaveRemoved->setChecked(
        m_settings->fileValue(Settings::Core::Internal::AutoExportPlaylistsSaveRemoved, false).toBool());
    m_autoExportSaveRemoved->setEnabled(!m_autoExportRemove->isChecked());
}

void PlaylistSavingPageWidget::apply()
{
    m_settings->fileSet(Settings::Core::Internal::PlaylistSavePathType, m_exportPathType->currentIndex());
    m_settings->fileSet(Settings::Core::Internal::PlaylistSaveMetadata, m_exportMetadata->isChecked());
    m_settings->fileSet(Settings::Core::Internal::AutoExportPlaylists, m_autoExporting->isChecked());
    m_settings->fileSet(Settings::Core::Internal::AutoExportPlaylistsType, m_autoExportType->currentText());
    m_settings->fileSet(Settings::Core::Internal::AutoExportPlaylistsPath, m_autoExportPath->text());
    m_settings->fileSet(Settings::Core::Internal::AutoExportPlaylistsRemove, m_autoExportRemove->isChecked());
    m_settings->fileSet(Settings::Core::Internal::AutoExportPlaylistsSaveRemoved, m_autoExportSaveRemoved->isChecked());
}

void PlaylistSavingPageWidget::reset()
{
    m_settings->fileRemove(Settings::Core::Internal::PlaylistSavePathType);
    m_settings->fileRemove(Settings::Core::Internal::PlaylistSaveMetadata);
    m_settings->fileRemove(Settings::Core::Internal::AutoExportPlaylists);
    m_settings->fileRemove(Settings::Core::Internal::AutoExportPlaylistsType);
    m_settings->fileRemove(Settings::Core::Internal::AutoExportPlaylistsPath);
    m_settings->fileRemove(Settings::Core::Internal::AutoExportPlaylistsRemove);
    m_settings->fileRemove(Settings::Core::Internal::AutoExportPlaylistsSaveRemoved);
}

void PlaylistSavingPageWidget::browseExportPath()
{
    const QString path = !m_autoExportPath->text().isEmpty() ? m_autoExportPath->text() : Core::playlistsPath();
    const QString dir
        = QFileDialog::getExistingDirectory(this, tr("Select Directory"), path, QFileDialog::DontResolveSymlinks);
    if(!dir.isEmpty()) {
        m_autoExportPath->setText(dir);
    }
}

PlaylistSavingPage::PlaylistSavingPage(const QStringList& playlistExtensions, SettingsManager* settings,
                                       QObject* parent)
    : SettingsPage{settings->settingsDialog(), parent}
{
    setId(Constants::Page::PlaylistSaving);
    setName(tr("Saving"));
    setCategory({tr("Playlist")});
    setWidgetCreator(
        [playlistExtensions, settings] { return new PlaylistSavingPageWidget(playlistExtensions, settings); });
}
} // namespace Fooyin

#include "moc_playlistsavingpage.cpp"
#include "playlistsavingpage.moc"