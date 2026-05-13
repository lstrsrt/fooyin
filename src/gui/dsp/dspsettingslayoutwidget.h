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

#include "dspsettingscontroller.h"

#include <gui/fywidget.h>

class QCheckBox;
class QComboBox;

namespace Fooyin {
class DspSettingsLayoutWidget : public FyWidget
{
public:
    DspSettingsLayoutWidget(DspSettingsController* controller, QString dspId, QString displayName, QWidget* parent);

    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString layoutName() const override;
    void saveLayoutData(QJsonObject& layout) override;
    void loadLayoutData(const QJsonObject& layout) override;

private:
    void refreshInstances();
    void selectInstance(const uint64_t instanceId);
    void clearInstance();

    DspSettingsController* m_controller;
    QString m_dspId;
    QString m_displayName;
    DspSettingsDialog* m_editor;
    QComboBox* m_instanceSelector;
    QCheckBox* m_enabledToggle;
    QMetaObject::Connection m_previewConnection;
    QMetaObject::Connection m_enabledConnection;
    uint64_t m_currentInstanceId;
};
} // namespace Fooyin