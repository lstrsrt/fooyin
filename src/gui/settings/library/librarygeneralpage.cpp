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

#include "librarygeneralpage.h"

#include "librarymodel.h"

#include "core/internalcoresettings.h"
#include <gui/iconloader.h>

#include "core/library/libraryinfo.h"
#include <core/coresettings.h>
#include <core/library/musiclibrary.h>
#include <gui/guiconstants.h>
#include <gui/widgets/extendabletableview.h>
#include <utils/settings/settingsmanager.h>
#include <utils/utils.h>

#include <QCheckBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>

using namespace Qt::StringLiterals;

namespace Fooyin {
class LibraryTableView : public ExtendableTableView
{
    Q_OBJECT

public:
    using ExtendableTableView::ExtendableTableView;

protected:
    void setupContextActions(QMenu* menu, const QPoint& pos) override;

Q_SIGNALS:
    void refreshLibrary(const Fooyin::LibraryInfo& info);
    void rescanLibrary(const Fooyin::LibraryInfo& info);
    void cancelLibraryScan(int id);
};

void LibraryTableView::setupContextActions(QMenu* menu, const QPoint& pos)
{
    const QModelIndex index = indexAt(pos);
    if(!index.isValid()) {
        return;
    }

    const auto library    = index.data(LibraryModel::Info).value<LibraryInfo>();
    const int scanId      = index.data(LibraryModel::ScanRequestId).toInt();
    const bool isPending  = library.status == LibraryInfo::Status::Pending;
    const bool isScanning = library.status == LibraryInfo::Status::Scanning && scanId >= 0;

    auto* refresh = new QAction(tr("&Scan for changes"), menu);
    refresh->setEnabled(!isPending && !isScanning);
    QObject::connect(refresh, &QAction::triggered, this, [this, library]() { Q_EMIT refreshLibrary(library); });

    auto* rescan = new QAction(tr("&Reload tracks"), menu);
    rescan->setEnabled(!isPending && !isScanning);
    QObject::connect(rescan, &QAction::triggered, this, [this, library]() { Q_EMIT rescanLibrary(library); });

    menu->addAction(refresh);
    menu->addAction(rescan);

    if(isScanning) {
        auto* cancel = new QAction(tr("&Cancel scan"), menu);
        Gui::setThemeIcon(cancel, Constants::Icons::Close);
        QObject::connect(cancel, &QAction::triggered, this, [this, scanId]() { Q_EMIT cancelLibraryScan(scanId); });
        menu->addSeparator();
        menu->addAction(cancel);
    }
}

class LibraryGeneralPageWidget : public SettingsPageWidget
{
    Q_OBJECT

public:
    explicit LibraryGeneralPageWidget(LibraryManager* libraryManager, MusicLibrary* library, SettingsManager* settings);

    void load() override;
    void apply() override;
    void reset() override;

private:
    void addLibrary() const;

    LibraryManager* m_libraryManager;
    MusicLibrary* m_library;
    SettingsManager* m_settings;

    LibraryTableView* m_libraryView;
    LibraryModel* m_model;

    QLineEdit* m_restrictTypes;
    QLineEdit* m_excludeTypes;

    QCheckBox* m_autoRefresh;
    QCheckBox* m_monitorLibraries;
    QCheckBox* m_markUnavailable;
    QCheckBox* m_markUnavailableStart;
    QCheckBox* m_useVariousCompilations;
    QCheckBox* m_saveRatings;
    QCheckBox* m_savePlaycounts;
    QCheckBox* m_overwriteRatingsOnReload;
    QCheckBox* m_overwritePlaycountsOnReload;
};

LibraryGeneralPageWidget::LibraryGeneralPageWidget(LibraryManager* libraryManager, MusicLibrary* library,
                                                   SettingsManager* settings)
    : m_libraryManager{libraryManager}
    , m_library{library}
    , m_settings{settings}
    , m_libraryView{new LibraryTableView(this)}
    , m_model{new LibraryModel(m_libraryManager, this)}
    , m_restrictTypes{new QLineEdit(this)}
    , m_excludeTypes{new QLineEdit(this)}
    , m_autoRefresh{new QCheckBox(tr("Auto refresh on startup"), this)}
    , m_monitorLibraries{new QCheckBox(tr("Monitor libraries"), this)}
    , m_markUnavailable{new QCheckBox(tr("Mark unavailable tracks on playback"), this)}
    , m_markUnavailableStart{new QCheckBox(tr("Mark unavailable tracks on startup"), this)}
    , m_useVariousCompilations{new QCheckBox(tr("Use 'Various Artists' for compilations"), this)}
    , m_saveRatings{new QCheckBox(tr("Save ratings to file tags when possible"), this)}
    , m_savePlaycounts{new QCheckBox(tr("Save playcounts to file tags when possible"), this)}
    , m_overwriteRatingsOnReload{new QCheckBox(tr("Overwrite rating in database when songs are re-read"), this)}
    , m_overwritePlaycountsOnReload{new QCheckBox(tr("Overwrite playcount in database when files are re-read"), this)}
{
    m_libraryView->setExtendableModel(m_model);

    // Hide Id column
    m_libraryView->hideColumn(0);

    m_libraryView->setExtendableColumn(1);
    m_libraryView->verticalHeader()->hide();
    m_libraryView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    m_autoRefresh->setToolTip(tr("Scan libraries for changes on startup"));
    m_monitorLibraries->setToolTip(tr("Monitor libraries for external changes"));

    auto* fileTypesGroup  = new QGroupBox(tr("File Types"), this);
    auto* fileTypesLayout = new QGridLayout(fileTypesGroup);

    int row{0};
    fileTypesLayout->addWidget(new QLabel(tr("Restrict to") + ":"_L1, this), row, 0);
    fileTypesLayout->addWidget(m_restrictTypes, row++, 1);
    fileTypesLayout->addWidget(new QLabel(tr("Exclude") + ":"_L1, this), row, 0);
    fileTypesLayout->addWidget(m_excludeTypes, row++, 1);
    //: Example of semicolon-separated file extensions (e.g. mp3;m4a)
    fileTypesLayout->addWidget(new QLabel(u"🛈 "_s + tr("e.g. \"%1\"").arg("mp3;m4a"_L1), this), row++, 1);
    fileTypesLayout->setColumnStretch(1, 1);

    auto* scanningGroup  = new QGroupBox(tr("Scanning"), this);
    auto* scanningLayout = new QGridLayout(scanningGroup);

    row = 0;
    scanningLayout->addWidget(m_autoRefresh, row++, 0);
    scanningLayout->addWidget(m_monitorLibraries, row++, 0);

    auto* availabilityGroup  = new QGroupBox(tr("Availability"), this);
    auto* availabilityLayout = new QGridLayout(availabilityGroup);

    row = 0;
    availabilityLayout->addWidget(m_markUnavailable, row++, 0);
    availabilityLayout->addWidget(m_markUnavailableStart, row++, 0);

    auto* metadataGroup  = new QGroupBox(tr("Metadata"), this);
    auto* metadataLayout = new QGridLayout(metadataGroup);

    row = 0;
    metadataLayout->addWidget(m_useVariousCompilations, row++, 0);

    auto* playbackStatsGroup  = new QGroupBox(tr("Playback Statistics"), this);
    auto* playbackStatsLayout = new QGridLayout(playbackStatsGroup);

    m_overwriteRatingsOnReload->setToolTip(tr(
        "When enabled, a rating found in file tags replaces the database rating.\n"
        "When disabled, the database rating is kept and file tags are only used when the database rating is empty."));
    m_overwritePlaycountsOnReload->setToolTip(
        tr("When enabled, playcount and played timestamps found in file tags replace the database values.\n"
           "Missing values still fall back to the database.\n"
           "When disabled, playcount uses the higher value, first played uses the earlier non-empty value,\n"
           "and last played uses the later value."));

    row = 0;
    playbackStatsLayout->addWidget(m_savePlaycounts, row++, 0);
    playbackStatsLayout->addWidget(m_saveRatings, row++, 0);
    playbackStatsLayout->addWidget(m_overwritePlaycountsOnReload, row++, 0);
    playbackStatsLayout->addWidget(m_overwriteRatingsOnReload, row++, 0);

    auto* mainLayout = new QGridLayout(this);

    row = 0;
    mainLayout->addWidget(m_libraryView, row++, 0, 1, 2);
    mainLayout->addWidget(fileTypesGroup, row++, 0, 1, 2);
    mainLayout->addWidget(scanningGroup, row, 0);
    mainLayout->addWidget(availabilityGroup, row++, 1);
    mainLayout->addWidget(metadataGroup, row++, 0, 1, 2);
    mainLayout->addWidget(playbackStatsGroup, row++, 0, 1, 2);
    mainLayout->setColumnStretch(0, 1);
    mainLayout->setColumnStretch(1, 1);

    QObject::connect(m_model, &LibraryModel::requestAddLibrary, this, &LibraryGeneralPageWidget::addLibrary);
    QObject::connect(m_libraryView, &LibraryTableView::refreshLibrary, m_library, &MusicLibrary::refresh);
    QObject::connect(m_libraryView, &LibraryTableView::rescanLibrary, m_library, &MusicLibrary::rescan);
    QObject::connect(m_libraryView, &LibraryTableView::cancelLibraryScan, m_library, &MusicLibrary::cancelScan);
    QObject::connect(m_library, &MusicLibrary::scanProgress, m_model, &LibraryModel::setScanProgress);
}

void LibraryGeneralPageWidget::load()
{
    m_model->populate();

    const QStringList restrictExtensions
        = m_settings->fileValue(Settings::Core::Internal::LibraryRestrictTypes).toStringList();
    const QStringList excludeExtensions
        = m_settings->fileValue(Settings::Core::Internal::LibraryExcludeTypes, QStringList{u"cue"_s}).toStringList();

    m_restrictTypes->setText(restrictExtensions.join(u';'));
    m_excludeTypes->setText(excludeExtensions.join(u';'));

    m_autoRefresh->setChecked(m_settings->value<Settings::Core::AutoRefresh>());
    m_monitorLibraries->setChecked(m_settings->value<Settings::Core::Internal::MonitorLibraries>());
    m_markUnavailable->setChecked(m_settings->fileValue(Settings::Core::Internal::MarkUnavailable, false).toBool());
    m_markUnavailableStart->setChecked(
        m_settings->fileValue(Settings::Core::Internal::MarkUnavailableStartup, false).toBool());
    m_useVariousCompilations->setChecked(m_settings->value<Settings::Core::UseVariousForCompilations>());
    m_saveRatings->setChecked(m_settings->value<Settings::Core::SaveRatingToMetadata>());
    m_savePlaycounts->setChecked(m_settings->value<Settings::Core::SavePlaycountToMetadata>());
    m_overwriteRatingsOnReload->setChecked(m_settings->value<Settings::Core::OverwriteRatingOnReload>());
    m_overwritePlaycountsOnReload->setChecked(m_settings->value<Settings::Core::OverwritePlaycountOnReload>());
}

void LibraryGeneralPageWidget::apply()
{
    m_model->processQueue();

    m_settings->fileSet(Settings::Core::Internal::LibraryRestrictTypes,
                        m_restrictTypes->text().split(u';', Qt::SkipEmptyParts));
    m_settings->fileSet(Settings::Core::Internal::LibraryExcludeTypes,
                        m_excludeTypes->text().split(u';', Qt::SkipEmptyParts));

    m_settings->set<Settings::Core::AutoRefresh>(m_autoRefresh->isChecked());
    m_settings->set<Settings::Core::Internal::MonitorLibraries>(m_monitorLibraries->isChecked());
    m_settings->fileSet(Settings::Core::Internal::MarkUnavailable, m_markUnavailable->isChecked());
    m_settings->fileSet(Settings::Core::Internal::MarkUnavailableStartup, m_markUnavailableStart->isChecked());
    m_settings->set<Settings::Core::UseVariousForCompilations>(m_useVariousCompilations->isChecked());
    m_settings->set<Settings::Core::SaveRatingToMetadata>(m_saveRatings->isChecked());
    m_settings->set<Settings::Core::SavePlaycountToMetadata>(m_savePlaycounts->isChecked());
    m_settings->set<Settings::Core::OverwriteRatingOnReload>(m_overwriteRatingsOnReload->isChecked());
    m_settings->set<Settings::Core::OverwritePlaycountOnReload>(m_overwritePlaycountsOnReload->isChecked());
}

void LibraryGeneralPageWidget::reset()
{
    m_settings->fileRemove(Settings::Core::Internal::LibraryRestrictTypes);
    m_settings->fileRemove(Settings::Core::Internal::LibraryExcludeTypes);

    m_settings->reset<Settings::Core::AutoRefresh>();
    m_settings->reset<Settings::Core::Internal::MonitorLibraries>();
    m_settings->fileRemove(Settings::Core::Internal::MarkUnavailable);
    m_settings->fileRemove(Settings::Core::Internal::MarkUnavailableStartup);
    m_settings->reset<Settings::Core::UseVariousForCompilations>();
    m_settings->reset<Settings::Core::SaveRatingToMetadata>();
    m_settings->reset<Settings::Core::SavePlaycountToMetadata>();
    m_settings->reset<Settings::Core::OverwriteRatingOnReload>();
    m_settings->reset<Settings::Core::OverwritePlaycountOnReload>();
}

void LibraryGeneralPageWidget::addLibrary() const
{
    const QString dir = QFileDialog::getExistingDirectory(m_libraryView, tr("Directory"), QDir::homePath(),
                                                          QFileDialog::DontResolveSymlinks);

    if(dir.isEmpty()) {
        m_model->markForAddition({});
        return;
    }

    const QFileInfo info{dir};
    const QString name = info.fileName();

    m_model->markForAddition({.name = name, .path = dir});
}

LibraryGeneralPage::LibraryGeneralPage(LibraryManager* libraryManager, MusicLibrary* library, SettingsManager* settings,
                                       QObject* parent)
    : SettingsPage{settings->settingsDialog(), parent}
{
    setId(Constants::Page::LibraryGeneral);
    setName(tr("General"));
    setCategory({tr("Library")});
    setWidgetCreator([libraryManager, library, settings] {
        return new LibraryGeneralPageWidget(libraryManager, library, settings);
    });
}
} // namespace Fooyin

#include "librarygeneralpage.moc"
#include "moc_librarygeneralpage.cpp"
