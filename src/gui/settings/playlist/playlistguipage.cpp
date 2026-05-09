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

#include "playlistguipage.h"

#include "internalguisettings.h"

#include <gui/guiconstants.h>
#include <utils/settings/settingsmanager.h>

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>

using namespace Qt::StringLiterals;

namespace Fooyin {
class PlaylistAppearancePageWidget : public SettingsPageWidget
{
    Q_OBJECT

public:
    explicit PlaylistAppearancePageWidget(SettingsManager* settings);

    void load() override;
    void apply() override;
    void reset() override;

private:
    SettingsManager* m_settings;

    QCheckBox* m_scrollBars;
    QCheckBox* m_header;
    QCheckBox* m_altColours;
    QSpinBox* m_imagePadding;
    QSpinBox* m_imagePaddingTop;
};

PlaylistAppearancePageWidget::PlaylistAppearancePageWidget(SettingsManager* settings)
    : m_settings{settings}
    , m_scrollBars{new QCheckBox(tr("Show scrollbar"), this)}
    , m_header{new QCheckBox(tr("Show header"), this)}
    , m_altColours{new QCheckBox(tr("Alternating row colours"), this)}
    , m_imagePadding{new QSpinBox(this)}
    , m_imagePaddingTop{new QSpinBox(this)}
{
    m_imagePadding->setMinimum(0);
    m_imagePadding->setMaximum(100);
    m_imagePadding->setSuffix(u" px"_s);

    m_imagePaddingTop->setMinimum(0);
    m_imagePaddingTop->setMaximum(100);
    m_imagePaddingTop->setSuffix(u" px"_s);

    auto* padding       = new QGroupBox(tr("Image Padding"), this);
    auto* paddingLayout = new QGridLayout(padding);

    int row{0};
    paddingLayout->addWidget(new QLabel(tr("Left/Right") + u":"_s, this), row, 0);
    paddingLayout->addWidget(m_imagePadding, row++, 1);
    paddingLayout->addWidget(new QLabel(tr("Top") + u":"_s, this), row, 0);
    paddingLayout->addWidget(m_imagePaddingTop, row++, 1);
    paddingLayout->setColumnStretch(2, 1);

    auto* appearance       = new QGroupBox(tr("Appearance"), this);
    auto* appearanceLayout = new QGridLayout(appearance);

    row = 0;
    appearanceLayout->addWidget(m_scrollBars, row++, 0, 1, 2);
    appearanceLayout->addWidget(m_header, row++, 0, 1, 2);
    appearanceLayout->addWidget(m_altColours, row++, 0, 1, 2);
    appearanceLayout->addWidget(padding, row, 0, 1, 3);
    appearanceLayout->setColumnStretch(2, 1);

    auto* mainLayout = new QGridLayout(this);
    mainLayout->addWidget(appearance, 0, 0);
    mainLayout->setRowStretch(1, 1);
}

void PlaylistAppearancePageWidget::load()
{
    m_scrollBars->setChecked(m_settings->value<Settings::Gui::Internal::PlaylistScrollBar>());
    m_header->setChecked(m_settings->value<Settings::Gui::Internal::PlaylistHeader>());
    m_altColours->setChecked(m_settings->value<Settings::Gui::Internal::PlaylistAltColours>());
    m_imagePadding->setValue(m_settings->value<Settings::Gui::Internal::PlaylistImagePadding>());
    m_imagePaddingTop->setValue(m_settings->value<Settings::Gui::Internal::PlaylistImagePaddingTop>());
}

void PlaylistAppearancePageWidget::apply()
{
    m_settings->set<Settings::Gui::Internal::PlaylistScrollBar>(m_scrollBars->isChecked());
    m_settings->set<Settings::Gui::Internal::PlaylistHeader>(m_header->isChecked());
    m_settings->set<Settings::Gui::Internal::PlaylistAltColours>(m_altColours->isChecked());
    m_settings->set<Settings::Gui::Internal::PlaylistImagePadding>(m_imagePadding->value());
    m_settings->set<Settings::Gui::Internal::PlaylistImagePaddingTop>(m_imagePaddingTop->value());
}

void PlaylistAppearancePageWidget::reset()
{
    m_settings->reset<Settings::Gui::Internal::PlaylistScrollBar>();
    m_settings->reset<Settings::Gui::Internal::PlaylistHeader>();
    m_settings->reset<Settings::Gui::Internal::PlaylistAltColours>();
    m_settings->reset<Settings::Gui::Internal::PlaylistImagePadding>();
    m_settings->reset<Settings::Gui::Internal::PlaylistImagePaddingTop>();
}

PlaylistGuiPage::PlaylistGuiPage(SettingsManager* settings, QObject* parent)
    : SettingsPage{settings->settingsDialog(), parent}
{
    setId(Constants::Page::PlaylistAppearance);
    setName(tr("Appearance"));
    setCategory({tr("Playlist")});
    setWidgetCreator([settings] { return new PlaylistAppearancePageWidget(settings); });
}
} // namespace Fooyin

#include "moc_playlistguipage.cpp"
#include "playlistguipage.moc"
