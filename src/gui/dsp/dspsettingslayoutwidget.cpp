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

#include "dspsettingslayoutwidget.h"

#include "dspsettingsregistry.h"

#include <gui/dsp/dspsettingsdialog.h>

#include <QCheckBox>
#include <QComboBox>
#include <QJsonObject>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace Fooyin {
namespace {
QCheckBox* addEnabledToggle(DspSettingsDialog* editor, bool enabled)
{
    auto* toggle = new QCheckBox(DspSettingsController::tr("Enabled"), editor);
    toggle->setChecked(enabled);
    editor->addButtonRowWidget(toggle);

    return toggle;
}

QComboBox* addInstanceSelector(DspSettingsDialog* editor)
{
    auto* selector = new QComboBox(editor);
    selector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    editor->addButtonRowWidget(selector);
    return selector;
}
} // namespace

DspSettingsLayoutWidget::DspSettingsLayoutWidget(DspSettingsController* controller, QString dspId, QString displayName,
                                                 QWidget* parent)
    : FyWidget{parent}
    , m_controller{controller}
    , m_dspId{std::move(dspId)}
    , m_displayName{std::move(displayName)}
    , m_editor{m_controller->createEditor(m_dspId, this)}
    , m_instanceSelector(addInstanceSelector(m_editor))
    , m_enabledToggle(addEnabledToggle(m_editor, false))
    , m_currentInstanceId{0}
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_editor->setButtonsVisible(false);
    m_editor->setWindowFlags(Qt::Widget);
    layout->addWidget(m_editor);

    QObject::connect(m_instanceSelector, &QComboBox::currentIndexChanged, m_editor, [this](int index) {
        if(index >= 0) {
            selectInstance(m_instanceSelector->itemData(index).toULongLong());
        }
    });
    QObject::connect(m_controller, &DspSettingsController::dspInstancesChanged, this,
                     &DspSettingsLayoutWidget::refreshInstances);

    refreshInstances();
}

QString DspSettingsLayoutWidget::name() const
{
    return m_displayName;
}

QString DspSettingsLayoutWidget::layoutName() const
{
    return DspSettingsRegistry::layoutWidgetKey(m_dspId);
}

void DspSettingsLayoutWidget::saveLayoutData(QJsonObject& layout)
{
    if(m_currentInstanceId != 0) {
        layout["InstanceID"_L1] = QString::number(m_currentInstanceId);
    }
}

void DspSettingsLayoutWidget::loadLayoutData(const QJsonObject& layout)
{
    if(layout.contains("InstanceID"_L1)) {
        m_currentInstanceId = static_cast<uint64_t>(layout.value("InstanceID"_L1).toString().toULongLong());
        refreshInstances();
    }
}

void DspSettingsLayoutWidget::refreshInstances()
{
    const auto targets = m_controller->targetsFor(m_dspId);

    const QSignalBlocker blocker{m_instanceSelector};
    m_instanceSelector->clear();

    uint64_t selectedInstanceId{0};
    for(const auto& target : targets) {
        m_instanceSelector->addItem(target.label, QString::number(target.instanceId));

        if(selectedInstanceId == 0 || target.instanceId == m_currentInstanceId) {
            selectedInstanceId = target.instanceId;
        }
    }

    // Only show instance selector if we have more than one instance of a DSP
    m_instanceSelector->setVisible(targets.size() > 1);

    if(selectedInstanceId == 0) {
        clearInstance();
        return;
    }

    selectInstance(selectedInstanceId);
}

void DspSettingsLayoutWidget::selectInstance(const uint64_t instanceId)
{
    if(instanceId == 0) {
        return;
    }

    const auto target = m_controller->targetForInstance(m_dspId, instanceId);
    if(!target) {
        return;
    }

    m_currentInstanceId = instanceId;

    if(m_instanceSelector) {
        const int index = m_instanceSelector->findData(QString::number(instanceId));
        if(index >= 0 && m_instanceSelector->currentIndex() != index) {
            const QSignalBlocker blocker{m_instanceSelector};
            m_instanceSelector->setCurrentIndex(index);
        }
    }

    QObject::disconnect(m_previewConnection);
    QObject::disconnect(m_enabledConnection);

    {
        const QSignalBlocker blocker{m_enabledToggle};
        m_enabledToggle->setChecked(target->enabled);
    }

    m_editor->loadSettings(target->settings);
    m_editor->setEnabled(true);

    m_previewConnection = QObject::connect(m_editor, &DspSettingsDialog::previewSettingsChanged, m_editor,
                                           [controller = m_controller, scope = target->scope,
                                            instanceId = target->instanceId](const QByteArray& settings) {
                                               controller->updateDspSettings(scope, instanceId, settings, true);
                                           });
    m_enabledConnection
        = QObject::connect(m_enabledToggle, &QCheckBox::toggled, m_editor,
                           [controller = m_controller, scope = target->scope, instanceId = target->instanceId](
                               const bool enabled) { controller->setDspEnabled(scope, instanceId, enabled); });
}

void DspSettingsLayoutWidget::clearInstance()
{
    QObject::disconnect(m_previewConnection);
    QObject::disconnect(m_enabledConnection);

    m_currentInstanceId = 0;

    if(m_enabledToggle) {
        const QSignalBlocker blocker{m_enabledToggle};
        m_enabledToggle->setChecked(false);
    }
    if(m_editor) {
        m_editor->setEnabled(false);
    }
}

} // namespace Fooyin