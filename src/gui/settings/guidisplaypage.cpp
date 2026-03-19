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

#include "guidisplaypage.h"

#include "internalguisettings.h"

#include <core/internalcoresettings.h>
#include <gui/guiconstants.h>
#include <gui/guisettings.h>
#include <gui/widgets/scriptlineedit.h>
#include <utils/settings/settingsmanager.h>

#include <QButtonGroup>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>
#include <QStyleFactory>

using namespace Qt::StringLiterals;

namespace Fooyin {
using namespace Settings::Gui;
using namespace Settings::Gui::Internal;

class GuiDisplayPageWidget : public SettingsPageWidget
{
public:
    explicit GuiDisplayPageWidget(SettingsManager* settings);

    void load() override;
    void apply() override;
    void reset() override;

private:
    SettingsManager* m_settings;

    QComboBox* m_styles;

    QRadioButton* m_detectIconTheme;
    QRadioButton* m_lightTheme;
    QRadioButton* m_darkTheme;
    QRadioButton* m_systemTheme;

    ScriptLineEdit* m_titleScript;
    QSpinBox* m_vbrInterval;

    QRadioButton* m_preferPlaying;
    QRadioButton* m_preferSelection;

    QSpinBox* m_starRatingSize;
    QSpinBox* m_allocationSize;
};

GuiDisplayPageWidget::GuiDisplayPageWidget(SettingsManager* settings)
    : m_settings{settings}
    , m_styles{new QComboBox(this)}
    , m_detectIconTheme{new QRadioButton(tr("Auto-detect theme"), this)}
    , m_lightTheme{new QRadioButton(tr("Light"), this)}
    , m_darkTheme{new QRadioButton(tr("Dark"), this)}
    , m_systemTheme{new QRadioButton(tr("Use system icons"), this)}
    , m_titleScript{new ScriptLineEdit(this)}
    , m_vbrInterval{new QSpinBox(this)}
    , m_preferPlaying{new QRadioButton(tr("Prefer currently playing track"), this)}
    , m_preferSelection{new QRadioButton(tr("Prefer current selection"), this)}
    , m_starRatingSize{new QSpinBox(this)}
    , m_allocationSize{new QSpinBox(this)}
{
    auto* themeGroup       = new QGroupBox(tr("Theme"), this);
    auto* themeGroupLayout = new QGridLayout(themeGroup);

    auto* iconThemeBox       = new QGroupBox(tr("Icons"), themeGroup);
    auto* iconThemeBoxLayout = new QGridLayout(iconThemeBox);

    int row{0};
    iconThemeBoxLayout->addWidget(m_detectIconTheme, row++, 0, 1, 2);
    iconThemeBoxLayout->addWidget(m_lightTheme, row, 0);
    iconThemeBoxLayout->addWidget(m_darkTheme, row++, 1);
    iconThemeBoxLayout->addWidget(m_systemTheme, row++, 0, 1, 2);
    iconThemeBoxLayout->setColumnStretch(2, 1);

    row = 0;
    themeGroupLayout->addWidget(new QLabel(tr("Style") + ":"_L1, this), row, 0);
    themeGroupLayout->addWidget(m_styles, row++, 1);
    themeGroupLayout->addWidget(iconThemeBox, row++, 0, 1, 2);
    themeGroupLayout->setColumnStretch(1, 1);

    auto* nowPlayingGroup       = new QGroupBox(tr("Now Playing"), this);
    auto* nowPlayingGroupLayout = new QGridLayout(nowPlayingGroup);

    m_vbrInterval->setRange(100, 300000);
    m_vbrInterval->setSingleStep(250);
    m_vbrInterval->setSuffix(u" ms"_s);

    row = 0;
    nowPlayingGroupLayout->addWidget(new QLabel(tr("Window title") + u":"_s, this), row, 0);
    nowPlayingGroupLayout->addWidget(m_titleScript, row++, 1, 1, 2);
    nowPlayingGroupLayout->addWidget(new QLabel(tr("VBR update interval") + u":"_s, this), row, 0);
    nowPlayingGroupLayout->addWidget(m_vbrInterval, row++, 1);
    nowPlayingGroupLayout->setColumnStretch(2, 1);

    auto* infoGroupBox       = new QGroupBox(tr("Information Display"), this);
    auto* selectionGroup     = new QButtonGroup(this);
    auto* infoGroupBoxLayout = new QGridLayout(infoGroupBox);

    selectionGroup->addButton(m_preferPlaying);
    selectionGroup->addButton(m_preferSelection);

    m_starRatingSize->setRange(5, 30);
    m_starRatingSize->setSuffix(u" px"_s);

    row = 0;
    infoGroupBoxLayout->addWidget(new QLabel(tr("Selection info") + u":"_s, this), row++, 0, 1, 3);
    infoGroupBoxLayout->addWidget(m_preferPlaying, row++, 0, 1, 3);
    infoGroupBoxLayout->addWidget(m_preferSelection, row++, 0, 1, 3);
    infoGroupBoxLayout->addWidget(new QLabel(tr("Star size") + u":"_s, this), row, 0);
    infoGroupBoxLayout->addWidget(m_starRatingSize, row++, 1);
    infoGroupBoxLayout->setColumnStretch(2, 1);

    auto* imagesGroupBox       = new QGroupBox(tr("Images"), this);
    auto* imagesGroupBoxLayout = new QGridLayout(imagesGroupBox);

    m_allocationSize->setRange(0, 1024);
    m_allocationSize->setSuffix(u" MB"_s);

    row = 0;
    imagesGroupBoxLayout->addWidget(new QLabel(tr("Image allocation limit") + u":"_s, this), row, 0);
    imagesGroupBoxLayout->addWidget(m_allocationSize, row++, 1);
    imagesGroupBoxLayout->addWidget(new QLabel(tr("Set to '0' to disable the limit."), this), row++, 0, 1, 2);
    imagesGroupBoxLayout->setColumnStretch(2, 1);

    auto* mainLayout = new QGridLayout(this);

    row = 0;
    mainLayout->addWidget(themeGroup, row++, 0, 1, 2);
    mainLayout->addWidget(nowPlayingGroup, row++, 0, 1, 2);
    mainLayout->addWidget(infoGroupBox, row++, 0, 1, 2);
    mainLayout->addWidget(imagesGroupBox, row++, 0, 1, 2);
    mainLayout->setColumnStretch(1, 1);
    mainLayout->setRowStretch(mainLayout->rowCount(), 1);
}

void GuiDisplayPageWidget::load()
{
    m_styles->clear();
    m_styles->addItem(u"System default"_s);

    const QStringList keys = QStyleFactory::keys();
    for(const QString& key : keys) {
        m_styles->addItem(key);
    }

    const auto style = m_settings->value<Style>();
    if(!style.isEmpty()) {
        m_styles->setCurrentText(style);
    }
    else {
        m_styles->setCurrentIndex(0);
    }

    const auto iconTheme = static_cast<IconThemeOption>(m_settings->value<IconTheme>());
    switch(iconTheme) {
        case(IconThemeOption::AutoDetect):
            m_detectIconTheme->setChecked(true);
            break;
        case(IconThemeOption::System):
            m_systemTheme->setChecked(true);
            break;
        case(IconThemeOption::Light):
            m_lightTheme->setChecked(true);
            break;
        case(IconThemeOption::Dark):
            m_darkTheme->setChecked(true);
            break;
    }

    m_titleScript->setText(m_settings->value<WindowTitleTrackScript>());
    m_vbrInterval->setValue(m_settings->value<Settings::Core::Internal::VBRUpdateInterval>());

    const auto option = static_cast<SelectionDisplay>(m_settings->value<InfoDisplayPrefer>());
    if(option == SelectionDisplay::PreferPlaying) {
        m_preferPlaying->setChecked(true);
    }
    else {
        m_preferSelection->setChecked(true);
    }

    m_starRatingSize->setValue(m_settings->value<StarRatingSize>());
    m_allocationSize->setValue(m_settings->value<ImageAllocationLimit>());
}

void GuiDisplayPageWidget::apply()
{
    m_settings->set<Style>(m_styles->currentText());

    IconThemeOption iconThemeOption;
    if(m_detectIconTheme->isChecked()) {
        iconThemeOption = IconThemeOption::AutoDetect;
    }
    else if(m_lightTheme->isChecked()) {
        iconThemeOption = IconThemeOption::Light;
    }
    else if(m_darkTheme->isChecked()) {
        iconThemeOption = IconThemeOption::Dark;
    }
    else {
        iconThemeOption = IconThemeOption::System;
    }
    m_settings->set<IconTheme>(static_cast<int>(iconThemeOption));

    m_settings->set<WindowTitleTrackScript>(m_titleScript->text());
    m_settings->set<Settings::Core::Internal::VBRUpdateInterval>(m_vbrInterval->value());

    const SelectionDisplay option
        = m_preferPlaying->isChecked() ? SelectionDisplay::PreferPlaying : SelectionDisplay::PreferSelection;
    m_settings->set<InfoDisplayPrefer>(static_cast<int>(option));

    m_settings->set<StarRatingSize>(m_starRatingSize->value());
    m_settings->set<ImageAllocationLimit>(m_allocationSize->value());
}

void GuiDisplayPageWidget::reset()
{
    m_settings->reset<Style>();
    m_settings->reset<IconTheme>();
    m_settings->reset<WindowTitleTrackScript>();
    m_settings->reset<Settings::Core::Internal::VBRUpdateInterval>();
    m_settings->reset<InfoDisplayPrefer>();
    m_settings->reset<StarRatingSize>();
    m_settings->reset<ImageAllocationLimit>();
}

GuiDisplayPage::GuiDisplayPage(SettingsManager* settings, QObject* parent)
    : SettingsPage{settings->settingsDialog(), parent}
{
    setId(Constants::Page::InterfaceDisplay);
    setName(tr("Display"));
    setCategory({tr("Interface")});
    setWidgetCreator([settings] { return new GuiDisplayPageWidget(settings); });
}
} // namespace Fooyin
