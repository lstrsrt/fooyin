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

#include "equalisersettingswidget.h"

#include "equaliserdsp.h"

#include <core/coresettings.h>
#include <gui/widgets/tooltip.h>

#include <QComboBox>
#include <QDataStream>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSlider>
#include <QSpacerItem>
#include <QStyle>
#include <QTextStream>
#include <QTimerEvent>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

using namespace Qt::StringLiterals;

constexpr auto SliderScale        = 10;
constexpr auto PresetStoreVersion = 1;
constexpr auto PresetsSettingKey  = "DSP/EqualiserPresets";
constexpr auto LastPresetPathKey  = "DSP/EqualiserLastPresetPath";

namespace {
constexpr std::array<const char*, 18> BandLabels = {
    "55",   "77",   "110",  "156",  "220", "311", "440", "622", "880",
    "1.2K", "1.8K", "2.5K", "3.5K", "5K",  "7K",  "10K", "14K", "20K",
};

QSlider* makeGainSlider(QWidget* parent)
{
    auto* slider = new QSlider(Qt::Vertical, parent);

    slider->setRange(-20 * SliderScale, 20 * SliderScale);
    slider->setSingleStep(1);
    slider->setPageStep(5);
    slider->setTickInterval(50);
    slider->setTickPosition(QSlider::TicksLeft);
    slider->setMinimumHeight(220);
    slider->setMaximumHeight(220);

    return slider;
}

QLabel* makeBandLabel(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);

    label->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    return label;
}

QLabel* makeValueLabel(QWidget* parent)
{
    auto* label = new QLabel(parent);

    label->setText(QStringLiteral("-20.0"));
    label->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    return label;
}

QStyleOptionSlider sliderStyleOption(QSlider* slider)
{
    QStyleOptionSlider sliderOpt;
    sliderOpt.initFrom(slider);
    sliderOpt.orientation    = Qt::Vertical;
    sliderOpt.minimum        = slider->minimum();
    sliderOpt.maximum        = slider->maximum();
    sliderOpt.tickPosition   = slider->tickPosition();
    sliderOpt.tickInterval   = slider->tickInterval();
    sliderOpt.upsideDown     = !slider->invertedAppearance();
    sliderOpt.direction      = slider->layoutDirection();
    sliderOpt.pageStep       = slider->pageStep();
    sliderOpt.singleStep     = slider->singleStep();
    sliderOpt.sliderPosition = slider->value();
    sliderOpt.sliderValue    = slider->value();

    return sliderOpt;
}

int sliderHandleHeight(QSlider* slider)
{
    if(!slider) {
        return 0;
    }

    const QStyleOptionSlider sliderOpt = sliderStyleOption(slider);
    const QRect handleRect
        = slider->style()->subControlRect(QStyle::CC_Slider, &sliderOpt, QStyle::SC_SliderHandle, slider);
    return std::max(0, handleRect.height());
}

int scaleHandleHalfOffset(QSlider* slider)
{
    return static_cast<int>(std::lround(static_cast<double>(sliderHandleHeight(slider)) / 2.0));
}

int scaleHandleQuarterOffset(QSlider* slider)
{
    return static_cast<int>(std::lround(static_cast<double>(sliderHandleHeight(slider)) / 4.0));
}
} // namespace

namespace Fooyin::Equaliser {
class ScaleLabelsWidget : public QWidget
{
public:
    explicit ScaleLabelsWidget(QSlider* slider, QWidget* parent = nullptr)
        : QWidget{parent}
        , m_slider{slider}
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    void syncToSlider()
    {
        setFixedHeight(m_slider->height() + scaleHandleHalfOffset(m_slider));

        const QFontMetrics metrics{font()};
        const int width = std::max({metrics.horizontalAdvance(tr("+20 dB")), metrics.horizontalAdvance(tr("+0 dB")),
                                    metrics.horizontalAdvance(tr("-20 dB"))});
        setFixedWidth(width);
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        QWidget::paintEvent(event);

        if(!m_slider) {
            return;
        }

        const QFontMetrics metrics{font()};

        const QString topText    = u"+20 dB"_s;
        const QString middleText = u"+0 dB"_s;
        const QString bottomText = u"-20 dB"_s;

        const QRect topRect    = metrics.tightBoundingRect(topText);
        const QRect middleRect = metrics.tightBoundingRect(middleText);
        const QRect bottomRect = metrics.tightBoundingRect(bottomText);

        const auto baselineForCentre = [&middleRect](int centreY) {
            return static_cast<int>(std::lround(
                static_cast<double>(centreY)
                - ((static_cast<double>(middleRect.top()) + static_cast<double>(middleRect.bottom())) / 2.0)));
        };

        const int trackOffset    = scaleHandleHalfOffset(m_slider);
        const int topBaseline    = scaleHandleQuarterOffset(m_slider) - topRect.top();
        const int middleBaseline = baselineForCentre(m_slider->rect().center().y() + trackOffset);
        const int bottomBaseline = (m_slider->height() - 1 + trackOffset) - bottomRect.bottom();

        QPainter painter{this};
        painter.setPen(palette().color(QPalette::WindowText));

        const auto drawRightAlignedText = [&painter, &metrics, this](const int baseline, const QString& text) {
            const int x = width() - metrics.horizontalAdvance(text);
            painter.drawText(x, baseline, text);
        };

        drawRightAlignedText(topBaseline, topText);
        drawRightAlignedText(middleBaseline, middleText);
        drawRightAlignedText(bottomBaseline, bottomText);
    }

private:
    QSlider* m_slider;
};

EqualiserSettingsWidget::EqualiserSettingsWidget(QWidget* parent)
    : DspSettingsDialog{parent}
    , m_presetBox{new QComboBox(this)}
    , m_loadPresetButton{new QPushButton(tr("Load"), this)}
    , m_savePresetButton{new QPushButton(tr("Save"), this)}
    , m_deletePresetButton{new QPushButton(tr("Delete"), this)}
    , m_importPresetButton{new QPushButton(tr("Import"), this)}
    , m_exportPresetButton{new QPushButton(tr("Export"), this)}
    , m_selectedBandCombo{new QComboBox(this)}
    , m_selectedBandSpin{new QDoubleSpinBox(this)}
    , m_preampSlider{makeGainSlider(this)}
    , m_preampValueLabel{makeValueLabel(this)}
    , m_bandSliders{}
    , m_bandValueLabels{}
    , m_scaleTrackWidget{new ScaleLabelsWidget(m_preampSlider, this)}
{
    setWindowTitle(tr("Equaliser Settings"));

    setRestoreDefaultsVisible(false);

    auto* root = contentLayout();

    auto* stripWidget = new QWidget(this);
    stripWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto* row = new QHBoxLayout(stripWidget);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(7);

    std::vector<QWidget*> sliderColumns;
    std::vector<QLabel*> bandLabels;
    std::vector<QLabel*> valueLabels;
    int sliderColumnMinWidth{0};

    const auto addSliderColumn = [this, row, &sliderColumns, &bandLabels, &valueLabels, &sliderColumnMinWidth](
                                     QSlider* slider, QLabel* valueLabel, const QString& labelText) {
        auto* col = new QVBoxLayout();
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(6);
        auto* label = makeBandLabel(labelText, this);
        col->addWidget(label);
        col->addWidget(slider, 1, Qt::AlignHCenter);
        col->addWidget(valueLabel);

        auto* colWidget = new QWidget(this);
        colWidget->setLayout(col);
        colWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sliderColumnMinWidth = std::max({sliderColumnMinWidth, slider->sizeHint().width(), label->sizeHint().width(),
                                         valueLabel->sizeHint().width()});
        sliderColumns.push_back(colWidget);
        bandLabels.push_back(label);
        valueLabels.push_back(valueLabel);

        row->addWidget(colWidget, 0, Qt::AlignTop);
    };

    addSliderColumn(m_preampSlider, m_preampValueLabel, tr("Preamp"));
    connectSliderSignals(m_preampSlider, false);

    for(size_t i{0}; i < m_bandSliders.size(); ++i) {
        m_bandSliders[i]     = makeGainSlider(this);
        m_bandValueLabels[i] = makeValueLabel(this);
        addSliderColumn(m_bandSliders[i], m_bandValueLabels[i], QString::fromLatin1(BandLabels[i]));
        connectSliderSignals(m_bandSliders[i], true);
    }

    for(auto* colWidget : sliderColumns) {
        colWidget->setFixedWidth(sliderColumnMinWidth);
    }

    int bandLabelHeight{0};
    for(auto* label : bandLabels) {
        bandLabelHeight = std::max(bandLabelHeight, label->sizeHint().height());
    }
    for(auto* label : bandLabels) {
        label->setFixedHeight(bandLabelHeight);
    }

    int valueLabelHeight{0};
    for(auto* label : valueLabels) {
        valueLabelHeight = std::max(valueLabelHeight, label->sizeHint().height());
    }
    for(auto* label : valueLabels) {
        label->setFixedHeight(valueLabelHeight);
    }

    auto* scaleCol = new QVBoxLayout();
    scaleCol->setContentsMargins(0, 0, 0, 0);
    scaleCol->setSpacing(6);

    scaleCol->addSpacerItem(new QSpacerItem(0, bandLabelHeight, QSizePolicy::Minimum, QSizePolicy::Fixed));

    m_scaleTrackWidget->syncToSlider();

    scaleCol->addWidget(m_scaleTrackWidget, 0, Qt::AlignTop | Qt::AlignRight);
    scaleCol->addSpacerItem(new QSpacerItem(0, valueLabelHeight, QSizePolicy::Minimum, QSizePolicy::Fixed));

    row->addSpacing(4);
    row->addLayout(scaleCol);
    root->addWidget(stripWidget, 0, Qt::AlignHCenter | Qt::AlignTop);

    auto* controlsWidget = new QWidget(this);
    controlsWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto* controlsLayout = new QHBoxLayout(controlsWidget);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controlsLayout->setSpacing(6);

    auto* zeroButton      = new QPushButton(tr("Zero all"), this);
    auto* autoButton      = new QPushButton(tr("Auto level"), this);
    auto* bandEditorLabel = new QLabel(tr("Band:"), this);
    auto* presetsLabel    = new QLabel(tr("Presets:"), this);

    QObject::connect(zeroButton, &QPushButton::clicked, this, [this]() { zeroAll(); });
    QObject::connect(autoButton, &QPushButton::clicked, this, [this]() { autoLevel(); });

    for(const auto& bandName : BandLabels) {
        m_selectedBandCombo->addItem(QString::fromLatin1(bandName));
    }

    m_selectedBandSpin->setRange(-20.0, 20.0);
    m_selectedBandSpin->setSingleStep(0.1);
    m_selectedBandSpin->setDecimals(1);
    m_selectedBandSpin->setSuffix(tr(" dB"));

    m_presetBox->setEditable(true);
    m_presetBox->setMinimumContentsLength(18);
    m_presetBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_presetBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    controlsLayout->addWidget(zeroButton);
    controlsLayout->addWidget(autoButton);
    controlsLayout->addWidget(bandEditorLabel);
    controlsLayout->addWidget(m_selectedBandCombo);
    controlsLayout->addWidget(m_selectedBandSpin);
    controlsLayout->addStretch(1);
    controlsLayout->addWidget(presetsLabel);
    controlsLayout->addWidget(m_presetBox);
    controlsLayout->addWidget(m_loadPresetButton);
    controlsLayout->addWidget(m_savePresetButton);
    controlsLayout->addWidget(m_deletePresetButton);
    controlsLayout->addWidget(m_importPresetButton);
    controlsLayout->addWidget(m_exportPresetButton);

    controlsWidget->setFixedWidth(stripWidget->sizeHint().width());

    root->addWidget(controlsWidget, 0, Qt::AlignHCenter | Qt::AlignTop);
    root->addStretch(1);

    QObject::connect(m_loadPresetButton, &QPushButton::clicked, this, [this]() { loadPreset(); });
    QObject::connect(m_savePresetButton, &QPushButton::clicked, this, [this]() { savePreset(); });
    QObject::connect(m_deletePresetButton, &QPushButton::clicked, this, [this]() { deletePreset(); });
    QObject::connect(m_importPresetButton, &QPushButton::clicked, this, [this]() { importPreset(); });
    QObject::connect(m_exportPresetButton, &QPushButton::clicked, this, [this]() { exportPreset(); });
    QObject::connect(m_selectedBandCombo, &QComboBox::currentIndexChanged, this,
                     [this](int) { refreshSelectedBandEditor(); });
    QObject::connect(m_selectedBandSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        const int bandIndex = m_selectedBandCombo->currentIndex();
        if(bandIndex < 0 || std::cmp_greater_equal(bandIndex, m_bandSliders.size())) {
            return;
        }
        m_bandSliders[static_cast<size_t>(bandIndex)]->setValue(gainDbToSliderValue(value));
    });
    QObject::connect(m_presetBox, &QComboBox::currentTextChanged, this, [this]() { updatePresetButtons(); });

    loadStoredPresets();
    refreshPresets();
    refreshSelectedBandEditor();
    refreshValueLabels();

    if(auto* dialogLayout = layout()) {
        dialogLayout->setSizeConstraint(QLayout::SetFixedSize);
    }
}

void EqualiserSettingsWidget::loadSettings(const QByteArray& settings)
{
    EqualiserDsp dsp;
    dsp.loadSettings(settings);

    std::array<int, EqualiserDsp::BandCount> bandSliderValues{};
    for(size_t i{0}; i < m_bandSliders.size(); ++i) {
        bandSliderValues[i] = gainDbToSliderValue(dsp.bandDb(static_cast<int>(i)));
    }

    applySliderValues(gainDbToSliderValue(dsp.preampDb()), bandSliderValues);
}

QByteArray EqualiserSettingsWidget::saveSettings() const
{
    EqualiserDsp dsp;
    dsp.setPreampDb(sliderValueToGainDb(m_preampSlider->value()));
    for(size_t i{0}; i < m_bandSliders.size(); ++i) {
        dsp.setBandDb(static_cast<int>(i), sliderValueToGainDb(m_bandSliders[i]->value()));
    }

    return dsp.saveSettings();
}

void EqualiserSettingsWidget::connectSliderSignals(QSlider* slider, const bool refreshBandEditor)
{
    if(!slider) {
        return;
    }

    QObject::connect(slider, &QSlider::valueChanged, this, [this, slider, refreshBandEditor](int) {
        refreshTooltips();
        refreshValueLabels();
        if(refreshBandEditor) {
            refreshSelectedBandEditor();
        }
        m_previewTimer.start(PreviewDebounceMs, this);
        if(slider->isSliderDown()) {
            updateSliderToolTip(slider);
        }
    });

    QObject::connect(slider, &QSlider::sliderPressed, this, [this, slider]() { updateSliderToolTip(slider); });
    QObject::connect(slider, &QSlider::sliderMoved, this, [this, slider](int) { updateSliderToolTip(slider); });
    QObject::connect(slider, &QSlider::sliderReleased, this, [this]() { hideSliderToolTip(); });
}

void EqualiserSettingsWidget::applySliderValues(const int preampSliderValue,
                                                const std::array<int, 18>& bandSliderValues)
{
    std::vector<QSignalBlocker> signalBlockers;
    signalBlockers.reserve(1 + m_bandSliders.size());
    signalBlockers.emplace_back(m_preampSlider);
    for(auto* slider : m_bandSliders) {
        signalBlockers.emplace_back(slider);
    }

    m_preampSlider->setValue(preampSliderValue);
    for(size_t i{0}; i < m_bandSliders.size(); ++i) {
        m_bandSliders[i]->setValue(bandSliderValues[i]);
    }

    refreshTooltips();
    refreshValueLabels();
    refreshSelectedBandEditor();
}

void EqualiserSettingsWidget::restoreDefaults()
{
    zeroAll();
}

void EqualiserSettingsWidget::loadStoredPresets()
{
    m_presets.clear();

    const FySettings settings;
    auto serializedData = settings.value(QLatin1String(PresetsSettingKey)).toByteArray();
    if(serializedData.isEmpty()) {
        return;
    }

    serializedData = qUncompress(serializedData);
    if(serializedData.isEmpty()) {
        return;
    }

    QDataStream stream{&serializedData, QIODevice::ReadOnly};
    stream.setVersion(QDataStream::Qt_6_0);

    quint32 version{0};
    qint32 presetCount{0};
    stream >> version;
    stream >> presetCount;

    if(stream.status() != QDataStream::Ok || version != PresetStoreVersion || presetCount < 0) {
        return;
    }

    for(qint32 i{0}; i < presetCount; ++i) {
        PresetItem preset;
        stream >> preset.name;
        stream >> preset.settings;

        preset.name = preset.name.trimmed();
        if(stream.status() != QDataStream::Ok || preset.name.isEmpty() || preset.settings.isEmpty()) {
            m_presets.clear();
            return;
        }

        if(presetIndexByName(preset.name) < 0) {
            m_presets.push_back(std::move(preset));
        }
    }
}

void EqualiserSettingsWidget::saveStoredPresets() const
{
    FySettings settings;

    if(m_presets.empty()) {
        settings.remove(QLatin1String(PresetsSettingKey));
        return;
    }

    QByteArray serializedData;
    QDataStream stream{&serializedData, QIODevice::WriteOnly};
    stream.setVersion(QDataStream::Qt_6_0);

    stream << static_cast<quint32>(PresetStoreVersion);
    stream << static_cast<qint32>(m_presets.size());

    for(const auto& preset : m_presets) {
        stream << preset.name;
        stream << preset.settings;
    }

    settings.setValue(QLatin1String(PresetsSettingKey), qCompress(serializedData, 9));
}

int EqualiserSettingsWidget::presetIndexByName(const QString& name) const
{
    const QString trimmedName = name.trimmed();
    if(trimmedName.isEmpty()) {
        return -1;
    }

    for(size_t i{0}; i < m_presets.size(); ++i) {
        if(m_presets[i].name == trimmedName) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void EqualiserSettingsWidget::refreshPresets()
{
    const QString currentText = m_presetBox->currentText().trimmed();

    m_presetBox->clear();
    for(const auto& preset : m_presets) {
        m_presetBox->addItem(preset.name);
    }

    if(!currentText.isEmpty()) {
        const int idx = m_presetBox->findText(currentText);
        if(idx >= 0) {
            m_presetBox->setCurrentIndex(idx);
        }
        else {
            m_presetBox->setEditText(currentText);
        }
    }

    updatePresetButtons();
}

void EqualiserSettingsWidget::updatePresetButtons()
{
    const QString name   = m_presetBox->currentText().trimmed();
    const bool hasPreset = presetIndexByName(name) >= 0;

    m_loadPresetButton->setEnabled(hasPreset);
    m_deletePresetButton->setEnabled(hasPreset);
    m_savePresetButton->setEnabled(!name.isEmpty());
}

void EqualiserSettingsWidget::loadPreset()
{
    const int presetIndex = presetIndexByName(m_presetBox->currentText());
    if(presetIndex < 0) {
        return;
    }

    EqualiserDsp dsp;
    if(!dsp.loadSettings(m_presets.at(static_cast<size_t>(presetIndex)).settings)) {
        QMessageBox::warning(this, tr("Presets"), tr("Unable to load the selected preset."));
        return;
    }

    std::array<int, EqualiserDsp::BandCount> bandSliderValues{};
    for(size_t i{0}; i < m_bandSliders.size(); ++i) {
        bandSliderValues[i] = gainDbToSliderValue(dsp.bandDb(static_cast<int>(i)));
    }

    applySliderValues(gainDbToSliderValue(dsp.preampDb()), bandSliderValues);
    m_previewTimer.start(PreviewDebounceMs, this);
}

void EqualiserSettingsWidget::savePreset()
{
    const QString name = m_presetBox->currentText().trimmed();
    if(name.isEmpty()) {
        return;
    }

    const int existingIndex = presetIndexByName(name);
    if(existingIndex >= 0) {
        QMessageBox msg{QMessageBox::Question, tr("Preset already exists"),
                        tr("Preset \"%1\" already exists. Overwrite?").arg(name), QMessageBox::Yes | QMessageBox::No};
        if(msg.exec() != QMessageBox::Yes) {
            return;
        }

        m_presets[static_cast<size_t>(existingIndex)].settings = saveSettings();
    }
    else {
        PresetItem preset;
        preset.name     = name;
        preset.settings = saveSettings();
        m_presets.push_back(std::move(preset));
    }

    saveStoredPresets();
    refreshPresets();
    m_presetBox->setCurrentText(name);
}

void EqualiserSettingsWidget::deletePreset()
{
    const int presetIndex = presetIndexByName(m_presetBox->currentText());
    if(presetIndex < 0) {
        return;
    }

    m_presets.erase(m_presets.begin() + presetIndex);
    QString nextPresetName;
    if(std::cmp_less(presetIndex, m_presets.size())) {
        nextPresetName = m_presets[static_cast<size_t>(presetIndex)].name;
    }
    else if(!m_presets.empty()) {
        nextPresetName = m_presets.back().name;
    }

    saveStoredPresets();
    m_presetBox->setEditText(nextPresetName);
    refreshPresets();
}

void EqualiserSettingsWidget::importPreset()
{
    QSettings settings;
    const QString initialPath = settings.value(QLatin1String(LastPresetPathKey), QDir::homePath()).toString();

    const QString filePath = QFileDialog::getOpenFileName(this, tr("Import Equaliser Preset"), initialPath,
                                                          tr("Equaliser Preset (*.feq)"));
    if(filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Import Equaliser Preset"),
                             tr("Unable to open \"%1\" for reading.").arg(filePath));
        return;
    }

    std::array<int, EqualiserDsp::BandCount> gains{};
    int parsedBands{0};
    int lineNumber{0};

    QTextStream stream(&file);
    while(!stream.atEnd() && parsedBands < EqualiserDsp::BandCount) {
        const QString line = stream.readLine();
        ++lineNumber;

        const QString trimmed = line.trimmed();
        if(trimmed.isEmpty()) {
            continue;
        }

        bool ok{false};
        const int value = trimmed.toInt(&ok);
        if(!ok) {
            QMessageBox::warning(this, tr("Import Equaliser Preset"),
                                 tr("Invalid value on line %L1.").arg(lineNumber) + u"\n"_s
                                     + tr("The first %Ln non-empty line(s) must contain integer values.", nullptr,
                                          EqualiserDsp::BandCount));
            return;
        }

        gains[static_cast<size_t>(parsedBands)] = value;
        ++parsedBands;
    }

    if(parsedBands < EqualiserDsp::BandCount) {
        QMessageBox::warning(this, tr("Import Equaliser Preset"),
                             tr("The preset file contains %Ln band value(s).", nullptr, parsedBands) + u"\n"_s
                                 + tr("Expected %Ln band value(s).", nullptr, EqualiserDsp::BandCount));
        return;
    }

    std::array<int, EqualiserDsp::BandCount> bandSliderValues{};
    for(size_t i{0}; i < bandSliderValues.size(); ++i) {
        bandSliderValues[i] = gainDbToSliderValue(static_cast<double>(gains[i]));
    }

    applySliderValues(m_preampSlider->value(), bandSliderValues);
    settings.setValue(QLatin1String(LastPresetPathKey), QFileInfo(filePath).absolutePath());
    m_previewTimer.start(PreviewDebounceMs, this);
}

void EqualiserSettingsWidget::exportPreset()
{
    FySettings settings;
    const QString initialPath = settings.value(QLatin1String(LastPresetPathKey), QDir::homePath()).toString();

    QString filePath = QFileDialog::getSaveFileName(this, tr("Export Equaliser Preset"), initialPath,
                                                    tr("Equaliser Preset (*.feq)"));
    if(filePath.isEmpty()) {
        return;
    }

    if(!filePath.endsWith(QStringLiteral(".feq"), Qt::CaseInsensitive)) {
        filePath += QStringLiteral(".feq");
    }

    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("Export Equaliser Preset"),
                             tr("Unable to open \"%1\" for writing.").arg(filePath));
        return;
    }

    QTextStream stream(&file);
    for(auto* slider : m_bandSliders) {
        const int value = static_cast<int>(std::lround(sliderValueToGainDb(slider->value())));
        stream << value << '\n';
    }

    if(stream.status() != QTextStream::Ok) {
        QMessageBox::warning(this, tr("Export Equaliser Preset"),
                             tr("An error occurred while writing \"%1\".").arg(filePath));
        return;
    }

    settings.setValue(QLatin1String(LastPresetPathKey), QFileInfo(filePath).absolutePath());
}

void EqualiserSettingsWidget::zeroAll()
{
    m_preampSlider->setValue(0);

    for(auto* slider : m_bandSliders) {
        slider->setValue(0);
    }

    refreshTooltips();
}

void EqualiserSettingsWidget::autoLevel()
{
    double maxBandDb{0.0};

    for(auto* slider : m_bandSliders) {
        maxBandDb = std::max(maxBandDb, sliderValueToGainDb(slider->value()));
    }

    if(maxBandDb <= 0.0) {
        refreshTooltips();
        return;
    }

    for(auto* slider : m_bandSliders) {
        const double band = sliderValueToGainDb(slider->value());
        slider->setValue(gainDbToSliderValue(band - maxBandDb));
    }

    refreshTooltips();
}

int EqualiserSettingsWidget::gainDbToSliderValue(const double gainDb)
{
    const double clamped = std::clamp(gainDb, -20.0, 20.0);
    return static_cast<int>(std::lround(clamped * static_cast<double>(SliderScale)));
}

double EqualiserSettingsWidget::sliderValueToGainDb(const int sliderValue)
{
    return std::clamp(static_cast<double>(sliderValue) / static_cast<double>(SliderScale), -20.0, 20.0);
}

QString EqualiserSettingsWidget::gainTooltip(const double gainDb)
{
    const QString prefix = gainDb >= 0.0 ? QStringLiteral("+") : QString{};
    return prefix + QString::number(gainDb, 'f', 1) + tr(" dB");
}

QString EqualiserSettingsWidget::gainValueLabel(const int sliderValue)
{
    return QString::number(sliderValueToGainDb(sliderValue), 'f', 1);
}

void EqualiserSettingsWidget::refreshTooltips()
{
    m_preampSlider->setToolTip(gainTooltip(sliderValueToGainDb(m_preampSlider->value())));

    for(auto* slider : m_bandSliders) {
        slider->setToolTip(gainTooltip(sliderValueToGainDb(slider->value())));
    }
}

void EqualiserSettingsWidget::refreshValueLabels()
{
    m_preampValueLabel->setText(gainValueLabel(m_preampSlider->value()));

    for(size_t i{0}; i < m_bandSliders.size(); ++i) {
        m_bandValueLabels[i]->setText(gainValueLabel(m_bandSliders[i]->value()));
    }
}

void EqualiserSettingsWidget::updateSliderToolTip(QSlider* slider)
{
    if(!slider) {
        return;
    }

    if(!m_sliderToolTip) {
        m_sliderToolTip = new ToolTip(this);
    }

    m_sliderToolTip->setText(gainTooltip(sliderValueToGainDb(slider->value())));
    m_sliderToolTip->show();
    m_sliderToolTip->raise();

    const QPoint cursorPos = slider->mapFromGlobal(QCursor::pos());
    const int handleY      = std::clamp(cursorPos.y(), slider->rect().top(), slider->rect().bottom());
    const QPoint handlePos = slider->mapTo(this, QPoint(slider->rect().center().x(), handleY));

    const int tipHeight = m_sliderToolTip->height();
    const int tipWidth  = m_sliderToolTip->width();
    const int anchorY   = std::clamp(handlePos.y() + (tipHeight / 2), tipHeight, height());

    const QPoint rightAnchor = slider->mapTo(this, QPoint(slider->rect().right() + 6, anchorY));
    const QPoint leftAnchor  = slider->mapTo(this, QPoint(slider->rect().left() - 6, anchorY));

    if(rightAnchor.x() + tipWidth <= width()) {
        m_sliderToolTip->setPosition(rightAnchor, Qt::AlignLeft);
    }
    else {
        m_sliderToolTip->setPosition(leftAnchor, Qt::AlignRight);
    }
}

void EqualiserSettingsWidget::hideSliderToolTip()
{
    if(m_sliderToolTip) {
        m_sliderToolTip->hide();
    }
}

void EqualiserSettingsWidget::refreshSelectedBandEditor()
{
    const int bandIndex  = m_selectedBandCombo->currentIndex();
    const bool validBand = (bandIndex >= 0 && std::cmp_less(bandIndex, m_bandSliders.size()));

    m_selectedBandSpin->setEnabled(validBand);
    if(!validBand) {
        return;
    }

    const int sliderValue = m_bandSliders[static_cast<size_t>(bandIndex)]->value();
    const QSignalBlocker blockSpin(m_selectedBandSpin);
    m_selectedBandSpin->setValue(sliderValueToGainDb(sliderValue));
}

void EqualiserSettingsWidget::timerEvent(QTimerEvent* event)
{
    if(event->timerId() == m_previewTimer.timerId()) {
        m_previewTimer.stop();
        publishPreviewSettings();
        return;
    }

    DspSettingsDialog::timerEvent(event);
}

QString EqualiserSettingsProvider::id() const
{
    return QStringLiteral("fooyin.dsp.equaliser");
}

DspSettingsDialog* EqualiserSettingsProvider::createSettingsWidget(QWidget* parent)
{
    return new EqualiserSettingsWidget(parent);
}
} // namespace Fooyin::Equaliser
