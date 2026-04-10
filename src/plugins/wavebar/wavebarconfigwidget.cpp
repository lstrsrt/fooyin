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

#include "wavebarconfigwidget.h"

#include "settings/wavebarsettings.h"
#include "wavebarcolours.h"

#include <gui/widgets/colourbutton.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace Fooyin::WaveBar {
WaveBarConfigDialog::WaveBarConfigDialog(WaveBarWidget* waveBar, QWidget* parent)
    : WidgetConfigDialog{waveBar, tr("WaveBar Settings"), parent}
    , m_showLabels{new QCheckBox(tr("Show labels"), this)}
    , m_showRemainingTime{new QCheckBox(tr("Show remaining time"), this)}
    , m_minMax{new QCheckBox(tr("Min/Max"), this)}
    , m_rms{new QCheckBox(tr("RMS"), this)}
    , m_silence{new QCheckBox(tr("Silence"), this)}
    , m_downmixOff{new QRadioButton(tr("Off"), this)}
    , m_downmixStereo{new QRadioButton(tr("Stereo"), this)}
    , m_downmixMono{new QRadioButton(tr("Mono"), this)}
    , m_showCursor{new QCheckBox(tr("Show progress cursor"), this)}
    , m_channelScale{new QDoubleSpinBox(this)}
    , m_cursorWidth{new QSpinBox(this)}
    , m_barWidth{new QSpinBox(this)}
    , m_barGap{new QSpinBox(this)}
    , m_supersampleFactor{new QComboBox(this)}
    , m_maxScale{new QDoubleSpinBox(this)}
    , m_centreGap{new QSpinBox(this)}
    , m_colourGroup{new QGroupBox(tr("Custom colours"), this)}
    , m_bgUnplayed{new ColourButton(this)}
    , m_bgPlayed{new ColourButton(this)}
    , m_maxUnplayed{new ColourButton(this)}
    , m_maxPlayed{new ColourButton(this)}
    , m_maxBorder{new ColourButton(this)}
    , m_minUnplayed{new ColourButton(this)}
    , m_minPlayed{new ColourButton(this)}
    , m_minBorder{new ColourButton(this)}
    , m_rmsMaxUnplayed{new ColourButton(this)}
    , m_rmsMaxPlayed{new ColourButton(this)}
    , m_rmsMaxBorder{new ColourButton(this)}
    , m_rmsMinUnplayed{new ColourButton(this)}
    , m_rmsMinPlayed{new ColourButton(this)}
    , m_rmsMinBorder{new ColourButton(this)}
    , m_cursorColour{new ColourButton(this)}
    , m_seekingCursorColour{new ColourButton(this)}
    , m_cacheSizeLabel{new QLabel(this)}
    , m_numSamples{new QComboBox(this)}
    , m_clearCacheButton{new QPushButton(tr("Clear Cache"), this)}
{
    auto* appearanceGroup  = new QGroupBox(tr("Appearance"), this);
    auto* appearanceLayout = new QVBoxLayout(appearanceGroup);
    appearanceLayout->addWidget(m_showLabels);
    appearanceLayout->addWidget(m_showRemainingTime);

    auto* modeGroup  = new QGroupBox(tr("Display"), this);
    auto* modeLayout = new QVBoxLayout(modeGroup);
    m_silence->setToolTip(tr("Draw a line in place of silence"));
    modeLayout->addWidget(m_minMax);
    modeLayout->addWidget(m_rms);
    modeLayout->addWidget(m_silence);

    auto* downmixGroupBox = new QGroupBox(tr("Downmix"), this);
    auto* downmixGroup    = new QButtonGroup(this);
    auto* downmixLayout   = new QVBoxLayout(downmixGroupBox);
    downmixGroup->addButton(m_downmixOff);
    downmixGroup->addButton(m_downmixStereo);
    downmixGroup->addButton(m_downmixMono);
    downmixLayout->addWidget(m_downmixOff);
    downmixLayout->addWidget(m_downmixStereo);
    downmixLayout->addWidget(m_downmixMono);

    auto* cursorGroup       = new QGroupBox(tr("Cursor"), this);
    auto* cursorGroupLayout = new QGridLayout(cursorGroup);
    m_cursorWidth->setRange(1, 20);
    m_cursorWidth->setSuffix(u" px"_s);
    cursorGroupLayout->addWidget(m_showCursor, 0, 0, 1, 2);
    cursorGroupLayout->addWidget(new QLabel(tr("Cursor width") + u":"_s, this), 1, 0);
    cursorGroupLayout->addWidget(m_cursorWidth, 1, 1);
    cursorGroupLayout->setColumnStretch(2, 1);

    auto* scaleGroup       = new QGroupBox(tr("Scale"), this);
    auto* scaleGroupLayout = new QGridLayout(scaleGroup);
    m_channelScale->setRange(0.0, 1.0);
    m_channelScale->setSingleStep(0.1);
    m_channelScale->setPrefix(u"x"_s);
    m_maxScale->setRange(0.0, 2.0);
    m_maxScale->setSingleStep(0.25);
    m_maxScale->setPrefix(u"x"_s);

    const QString supersampleTip{
        tr("Internal horizontal render scale for the waveform.\n"
           "Higher values can improve detail, but may increase CPU usage.\n"
           "For best results at higher values, increase Number of samples as well.\n"
           "Supersampling is intended for 1 px bars with a 0 px gap; other bar sizes can produce artifacts.")};
    m_supersampleFactor->addItem(tr("Off"), 1);
    m_supersampleFactor->addItem(tr("2x"), 2);
    m_supersampleFactor->addItem(tr("4x"), 4);
    m_supersampleFactor->addItem(tr("8x"), 8);
    m_supersampleFactor->setToolTip(supersampleTip);

    auto* supersampleLabel = new QLabel(tr("Supersampling") + u":"_s, this);
    supersampleLabel->setToolTip(supersampleTip);

    scaleGroupLayout->addWidget(new QLabel(tr("Channel scale") + u":"_s, this), 0, 0);
    scaleGroupLayout->addWidget(m_channelScale, 0, 1);
    scaleGroupLayout->addWidget(new QLabel(tr("Max scale") + u":"_s, this), 1, 0);
    scaleGroupLayout->addWidget(m_maxScale, 1, 1);
    scaleGroupLayout->addWidget(supersampleLabel, 2, 0);
    scaleGroupLayout->addWidget(m_supersampleFactor, 2, 1);
    scaleGroupLayout->setColumnStretch(2, 1);

    auto* dimensionGroup       = new QGroupBox(tr("Dimension"), this);
    auto* dimensionGroupLayout = new QGridLayout(dimensionGroup);
    m_barWidth->setRange(1, 50);
    m_barWidth->setSuffix(u" px"_s);
    m_barGap->setRange(0, 50);
    m_barGap->setSuffix(u" px"_s);
    m_centreGap->setRange(0, 10);
    m_centreGap->setSuffix(u" px"_s);
    dimensionGroupLayout->addWidget(new QLabel(tr("Bar width") + u":"_s, this), 0, 0);
    dimensionGroupLayout->addWidget(m_barWidth, 0, 1);
    dimensionGroupLayout->addWidget(new QLabel(tr("Bar gap") + u":"_s, this), 1, 0);
    dimensionGroupLayout->addWidget(m_barGap, 1, 1);
    dimensionGroupLayout->addWidget(new QLabel(tr("Centre gap") + u":"_s, this), 2, 0);
    dimensionGroupLayout->addWidget(m_centreGap, 2, 1);
    dimensionGroupLayout->setColumnStretch(2, 1);

    m_colourGroup->setCheckable(true);
    auto* coloursLayout = new QGridLayout(m_colourGroup);

    int row{0};
    coloursLayout->addWidget(new QLabel(tr("Unplayed"), this), row, 1, Qt::AlignCenter);
    coloursLayout->addWidget(new QLabel(tr("Played"), this), row, 2, Qt::AlignCenter);
    coloursLayout->addWidget(new QLabel(tr("Border"), this), row++, 3, Qt::AlignCenter);
    coloursLayout->addWidget(new QLabel(tr("Background"), this), row, 0);
    coloursLayout->addWidget(m_bgUnplayed, row, 1);
    coloursLayout->addWidget(m_bgPlayed, row++, 2);
    coloursLayout->addWidget(new QLabel(tr("Max"), this), row, 0);
    coloursLayout->addWidget(m_maxUnplayed, row, 1);
    coloursLayout->addWidget(m_maxPlayed, row, 2);
    coloursLayout->addWidget(m_maxBorder, row++, 3);
    coloursLayout->addWidget(new QLabel(tr("Min"), this), row, 0);
    coloursLayout->addWidget(m_minUnplayed, row, 1);
    coloursLayout->addWidget(m_minPlayed, row, 2);
    coloursLayout->addWidget(m_minBorder, row++, 3);
    coloursLayout->addWidget(new QLabel(tr("RMS Max"), this), row, 0);
    coloursLayout->addWidget(m_rmsMaxUnplayed, row, 1);
    coloursLayout->addWidget(m_rmsMaxPlayed, row, 2);
    coloursLayout->addWidget(m_rmsMaxBorder, row++, 3);
    coloursLayout->addWidget(new QLabel(tr("RMS Min"), this), row, 0);
    coloursLayout->addWidget(m_rmsMinUnplayed, row, 1);
    coloursLayout->addWidget(m_rmsMinPlayed, row, 2);
    coloursLayout->addWidget(m_rmsMinBorder, row++, 3);
    coloursLayout->addWidget(new QLabel(tr("Playing"), this), row, 1, Qt::AlignCenter);
    coloursLayout->addWidget(new QLabel(tr("Seeking"), this), row++, 2, Qt::AlignCenter);
    coloursLayout->addWidget(new QLabel(tr("Cursor"), this), row, 0);
    coloursLayout->addWidget(m_cursorColour, row, 1);
    coloursLayout->addWidget(m_seekingCursorColour, row, 2);
    coloursLayout->setColumnStretch(1, 1);
    coloursLayout->setColumnStretch(2, 1);
    coloursLayout->setColumnStretch(3, 1);

    auto* cacheGroup       = new QGroupBox(tr("Cache"), this);
    auto* cacheGroupLayout = new QGridLayout(cacheGroup);
    const QString numSamplesTip{tr("Number of samples (per channel) to use for waveform data.\n"
                                   "Higher values produce a more accurate and detailed waveform, "
                                   "but use more disk space in the cache.\n"
                                   "Higher supersampling values work best with higher sample counts.")};
    auto* numSamplesLabel = new QLabel(tr("Number of samples") + u":"_s, this);

    m_numSamples->addItem(QString::number(2048), 2048);
    m_numSamples->addItem(QString::number(4096), 4096);
    m_numSamples->addItem(QString::number(8192), 8192);

    numSamplesLabel->setToolTip(numSamplesTip);
    m_numSamples->setToolTip(numSamplesTip);

    cacheGroupLayout->addWidget(numSamplesLabel, 1, 0);
    cacheGroupLayout->addWidget(m_numSamples, 1, 1);
    cacheGroupLayout->addWidget(m_cacheSizeLabel, 2, 0);
    cacheGroupLayout->addWidget(m_clearCacheButton, 2, 1);
    cacheGroupLayout->setColumnStretch(2, 1);

    auto* layout = contentLayout();

    layout->addWidget(appearanceGroup, 0, 0);
    layout->addWidget(modeGroup, 0, 1);
    layout->addWidget(downmixGroupBox, 1, 0);
    layout->addWidget(cursorGroup, 1, 1);
    layout->addWidget(dimensionGroup, 2, 0);
    layout->addWidget(scaleGroup, 2, 1);
    layout->addWidget(m_colourGroup, 3, 0, 1, 2);
    layout->addWidget(cacheGroup, 4, 0, 1, 2);
    layout->setRowStretch(layout->rowCount(), 1);

    QObject::connect(m_clearCacheButton, &QPushButton::clicked, this, [this]() {
        widget()->requestClearCache();
        updateCacheSize();
    });

    loadCurrentConfig();
    setGlobalCacheConfig(widget()->globalNumSamples());
    updateCacheSize();
}

WaveBarWidget::ConfigData WaveBarConfigDialog::config() const
{
    auto mode = WaveModes{};

    if(m_minMax->isChecked()) {
        mode |= MinMax;
    }
    if(m_rms->isChecked()) {
        mode |= Rms;
    }
    if(m_silence->isChecked()) {
        mode |= Silence;
    }

    WaveBarWidget::ConfigData config{
        .showLabels        = m_showLabels->isChecked(),
        .showRemainingTime = m_showRemainingTime->isChecked(),
        .showCursor        = m_showCursor->isChecked(),
        .cursorWidth       = m_cursorWidth->value(),
        .mode              = static_cast<int>(mode),
        .downmix  = m_downmixStereo->isChecked() ? static_cast<int>(DownmixOption::Stereo)
                                                 : (m_downmixMono->isChecked() ? static_cast<int>(DownmixOption::Mono)
                                                                               : static_cast<int>(DownmixOption::Off)),
        .barWidth = m_barWidth->value(),
        .barGap   = m_barGap->value(),
        .supersampleFactor = m_supersampleFactor->currentData().toInt(),
        .maxScale          = m_maxScale->value(),
        .centreGap         = m_centreGap->value(),
        .channelScale      = m_channelScale->value(),
        .colourOptions     = QVariant{},
    };

    if(m_colourGroup->isChecked()) {
        Colours colours;
        colours.setColour(Colours::Type::BgUnplayed, m_bgUnplayed->colour());
        colours.setColour(Colours::Type::BgPlayed, m_bgPlayed->colour());
        colours.setColour(Colours::Type::MaxUnplayed, m_maxUnplayed->colour());
        colours.setColour(Colours::Type::MaxPlayed, m_maxPlayed->colour());
        colours.setColour(Colours::Type::MaxBorder, m_maxBorder->colour());
        colours.setColour(Colours::Type::MinUnplayed, m_minUnplayed->colour());
        colours.setColour(Colours::Type::MinPlayed, m_minPlayed->colour());
        colours.setColour(Colours::Type::MinBorder, m_minBorder->colour());
        colours.setColour(Colours::Type::RmsMaxUnplayed, m_rmsMaxUnplayed->colour());
        colours.setColour(Colours::Type::RmsMaxPlayed, m_rmsMaxPlayed->colour());
        colours.setColour(Colours::Type::RmsMaxBorder, m_rmsMaxBorder->colour());
        colours.setColour(Colours::Type::RmsMinUnplayed, m_rmsMinUnplayed->colour());
        colours.setColour(Colours::Type::RmsMinPlayed, m_rmsMinPlayed->colour());
        colours.setColour(Colours::Type::RmsMinBorder, m_rmsMinBorder->colour());
        colours.setColour(Colours::Type::Cursor, m_cursorColour->colour());
        colours.setColour(Colours::Type::SeekingCursor, m_seekingCursorColour->colour());
        config.colourOptions = QVariant::fromValue(colours);
    }

    return config;
}

void WaveBarConfigDialog::setConfig(const WaveBarWidget::ConfigData& config)
{
    m_showLabels->setChecked(config.showLabels);
    m_showRemainingTime->setChecked(config.showRemainingTime);
    m_showCursor->setChecked(config.showCursor);
    m_cursorWidth->setValue(config.cursorWidth);
    m_channelScale->setValue(config.channelScale);
    m_barWidth->setValue(config.barWidth);
    m_barGap->setValue(config.barGap);
    const int supersampleIndex = m_supersampleFactor->findData(config.supersampleFactor);
    m_supersampleFactor->setCurrentIndex(supersampleIndex >= 0 ? supersampleIndex : 0);
    m_maxScale->setValue(config.maxScale);
    m_centreGap->setValue(config.centreGap);

    const auto mode = static_cast<WaveModes>(config.mode);
    m_minMax->setChecked(mode & MinMax);
    m_rms->setChecked(mode & Rms);
    m_silence->setChecked(mode & Silence);

    const auto downmix = static_cast<DownmixOption>(config.downmix);
    if(downmix == DownmixOption::Off) {
        m_downmixOff->setChecked(true);
    }
    else if(downmix == DownmixOption::Stereo) {
        m_downmixStereo->setChecked(true);
    }
    else {
        m_downmixMono->setChecked(true);
    }

    const bool customColours = config.colourOptions.isValid() && config.colourOptions.canConvert<Colours>()
                            && !config.colourOptions.value<Colours>().isEmpty();
    const Colours colours    = customColours ? config.colourOptions.value<Colours>() : Colours{};
    m_colourGroup->setChecked(customColours);

    m_bgUnplayed->setColour(colours.colour(Colours::Type::BgUnplayed, palette()));
    m_bgPlayed->setColour(colours.colour(Colours::Type::BgPlayed, palette()));
    m_maxUnplayed->setColour(colours.colour(Colours::Type::MaxUnplayed, palette()));
    m_maxPlayed->setColour(colours.colour(Colours::Type::MaxPlayed, palette()));
    m_maxBorder->setColour(colours.colour(Colours::Type::MaxBorder, palette()));
    m_minUnplayed->setColour(colours.colour(Colours::Type::MinUnplayed, palette()));
    m_minPlayed->setColour(colours.colour(Colours::Type::MinPlayed, palette()));
    m_minBorder->setColour(colours.colour(Colours::Type::MinBorder, palette()));
    m_rmsMaxUnplayed->setColour(colours.colour(Colours::Type::RmsMaxUnplayed, palette()));
    m_rmsMaxPlayed->setColour(colours.colour(Colours::Type::RmsMaxPlayed, palette()));
    m_rmsMaxBorder->setColour(colours.colour(Colours::Type::RmsMaxBorder, palette()));
    m_rmsMinUnplayed->setColour(colours.colour(Colours::Type::RmsMinUnplayed, palette()));
    m_rmsMinPlayed->setColour(colours.colour(Colours::Type::RmsMinPlayed, palette()));
    m_rmsMinBorder->setColour(colours.colour(Colours::Type::RmsMinBorder, palette()));
    m_cursorColour->setColour(colours.colour(Colours::Type::Cursor, palette()));
    m_seekingCursorColour->setColour(colours.colour(Colours::Type::SeekingCursor, palette()));
}

void WaveBarConfigDialog::apply()
{
    WidgetConfigDialog::apply();
    applyGlobalCacheConfig();
}

void WaveBarConfigDialog::saveDefaults()
{
    WidgetConfigDialog::saveDefaults();
    applyGlobalCacheConfig();
}

void WaveBarConfigDialog::restoreSavedDefaults()
{
    WidgetConfigDialog::restoreSavedDefaults();
    setGlobalCacheConfig(widget()->globalNumSamples());
    updateCacheSize();
}

void WaveBarConfigDialog::restoreFactoryDefaults()
{
    WidgetConfigDialog::restoreFactoryDefaults();
    setGlobalCacheConfig(2048);
    updateCacheSize();
}

void WaveBarConfigDialog::clearSavedDefaults()
{
    WidgetConfigDialog::clearSavedDefaults();
    setGlobalCacheConfig(2048);
    updateCacheSize();
}

void WaveBarConfigDialog::setGlobalCacheConfig(int numSamples)
{
    const int index = m_numSamples->findData(numSamples);
    m_numSamples->setCurrentIndex(index >= 0 ? index : 0);
}

int WaveBarConfigDialog::globalNumSamples() const
{
    return m_numSamples->currentData().toInt();
}

void WaveBarConfigDialog::updateCacheSize()
{
    m_cacheSizeLabel->setText(widget()->cacheSizeText());
}

void WaveBarConfigDialog::applyGlobalCacheConfig()
{
    if(widget()->setGlobalNumSamples(globalNumSamples())) {
        widget()->requestClearCache();
    }
    updateCacheSize();
}
} // namespace Fooyin::WaveBar
