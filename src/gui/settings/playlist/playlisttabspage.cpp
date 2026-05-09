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

#include "playlisttabspage.h"

#include "internalguisettings.h"

#include <gui/guiconstants.h>
#include <utils/settings/settingsmanager.h>

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>

namespace Fooyin {
class PlaylistTabsPageWidget : public SettingsPageWidget
{
    Q_OBJECT

public:
    explicit PlaylistTabsPageWidget(SettingsManager* settings);

    void load() override;
    void apply() override;
    void reset() override;

private:
    SettingsManager* m_settings;

    QCheckBox* m_tabsExpand;
    QCheckBox* m_tabsAddButton;
    QCheckBox* m_tabsClearButton;
    QCheckBox* m_tabsCloseButton;
    QCheckBox* m_tabsMiddleClose;
};

PlaylistTabsPageWidget::PlaylistTabsPageWidget(SettingsManager* settings)
    : m_settings{settings}
    , m_tabsExpand{new QCheckBox(tr("Expand tabs to fill empty space"), this)}
    , m_tabsAddButton{new QCheckBox(tr("Show add button"), this)}
    , m_tabsClearButton{new QCheckBox(tr("Show clear button"), this)}
    , m_tabsCloseButton{new QCheckBox(tr("Show delete button on tabs"), this)}
    , m_tabsMiddleClose{new QCheckBox(tr("Delete playlists on middle click"), this)}
{
    auto* tabsGroup       = new QGroupBox(tr("Playlist Tabs"), this);
    auto* tabsGroupLayout = new QGridLayout(tabsGroup);

    int row{0};
    tabsGroupLayout->addWidget(m_tabsExpand, row++, 0);
    tabsGroupLayout->addWidget(m_tabsAddButton, row++, 0);
    tabsGroupLayout->addWidget(m_tabsClearButton, row++, 0);
    tabsGroupLayout->addWidget(m_tabsCloseButton, row++, 0);
    tabsGroupLayout->addWidget(m_tabsMiddleClose, row++, 0);

    auto* mainLayout = new QGridLayout(this);
    mainLayout->addWidget(tabsGroup, 0, 0);
    mainLayout->setRowStretch(1, 1);
}

void PlaylistTabsPageWidget::load()
{
    m_tabsExpand->setChecked(m_settings->value<Settings::Gui::Internal::PlaylistTabsExpand>());
    m_tabsAddButton->setChecked(m_settings->value<Settings::Gui::Internal::PlaylistTabsAddButton>());
    m_tabsClearButton->setChecked(m_settings->value<Settings::Gui::Internal::PlaylistTabsClearButton>());
    m_tabsCloseButton->setChecked(m_settings->value<Settings::Gui::Internal::PlaylistTabsCloseButton>());
    m_tabsMiddleClose->setChecked(m_settings->value<Settings::Gui::Internal::PlaylistTabsMiddleClose>());
}

void PlaylistTabsPageWidget::apply()
{
    m_settings->set<Settings::Gui::Internal::PlaylistTabsExpand>(m_tabsExpand->isChecked());
    m_settings->set<Settings::Gui::Internal::PlaylistTabsAddButton>(m_tabsAddButton->isChecked());
    m_settings->set<Settings::Gui::Internal::PlaylistTabsClearButton>(m_tabsClearButton->isChecked());
    m_settings->set<Settings::Gui::Internal::PlaylistTabsCloseButton>(m_tabsCloseButton->isChecked());
    m_settings->set<Settings::Gui::Internal::PlaylistTabsMiddleClose>(m_tabsMiddleClose->isChecked());
}

void PlaylistTabsPageWidget::reset()
{
    m_settings->reset<Settings::Gui::Internal::PlaylistTabsExpand>();
    m_settings->reset<Settings::Gui::Internal::PlaylistTabsAddButton>();
    m_settings->reset<Settings::Gui::Internal::PlaylistTabsClearButton>();
    m_settings->reset<Settings::Gui::Internal::PlaylistTabsCloseButton>();
    m_settings->reset<Settings::Gui::Internal::PlaylistTabsMiddleClose>();
}

PlaylistTabsPage::PlaylistTabsPage(SettingsManager* settings, QObject* parent)
    : SettingsPage{settings->settingsDialog(), parent}
{
    setId(Constants::Page::PlaylistTabs);
    setName(tr("Tabs"));
    setCategory({tr("Playlist")});
    setWidgetCreator([settings] { return new PlaylistTabsPageWidget(settings); });
}
} // namespace Fooyin

#include "moc_playlisttabspage.cpp"
#include "playlisttabspage.moc"
