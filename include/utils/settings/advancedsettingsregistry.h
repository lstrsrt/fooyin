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

#include "fyutils_export.h"

#include <QVariant>

#include <functional>
#include <variant>
#include <vector>

namespace Fooyin {
/*!
 * Checkbox editor marker.
 *
 * The descriptor's current value is interpreted as a boolean and toggled directly in the tree row.
 */
struct AdvancedSettingCheckBox
{ };

struct AdvancedSettingOption
{
    QVariant value;
    QString label;
};
/*!
 * Radio-button editor configuration.
 *
 * Each option is shown as a child row. Selecting an option writes that option's value to the parent setting.
 */
struct AdvancedSettingRadioButtons
{
    std::vector<AdvancedSettingOption> options;
};
/*!
 * Line edit editor marker.
 *
 * The setting is displayed as "Label: value"; a line edit is created only while the row is being edited.
 */
struct AdvancedSettingLineEdit
{ };
/*!
 * Integer spin box editor configuration.
 *
 * The setting is displayed as "Label: value"; a spin box is created only while the row is being edited.
 */
struct AdvancedSettingSpinBox
{
    int minimum{0};
    int maximum{100};
    int singleStep{1};
    QString suffix;
};

using AdvancedSettingEditor = std::variant<AdvancedSettingCheckBox, AdvancedSettingRadioButtons,
                                           AdvancedSettingLineEdit, AdvancedSettingSpinBox>;

/*!
 * Runtime descriptor for a single advanced setting.
 *
 * Editor-specific metadata lives in the editor variant; read/write/normalise/validate provide the bridge to
 * the underlying settings store.
 */
struct AdvancedSettingDescriptor
{
    //! Stable identifier used to replace an existing descriptor when re-registering.
    QString id;
    //! Category path displayed in the Advanced settings tree.
    QStringList category;
    //! User-visible setting name.
    QString label;
    //! Optional tooltip text.
    QString description;
    //! Value written when the Advanced page is reset.
    QVariant defaultValue;
    //! Editor type and editor-specific metadata.
    AdvancedSettingEditor editor{AdvancedSettingLineEdit{}};
    //! Reads the current value from the backing store.
    std::function<QVariant()> read;
    //! Writes a value to the backing store. Return false if the write could not be applied.
    std::function<bool(const QVariant&)> write;
    //! Optional value normalisation applied before display, validation, and writing.
    std::function<QVariant(const QVariant&)> normalise;
    //! Optional validation hook. Return an empty string when the value is valid.
    std::function<QString(const QVariant&)> validate;
};

/*!
 * Registry of advanced settings.
 *
 * The registry does not own the backing settings. Callers provide read/write callbacks, and the Advanced page consumes
 * the registered descriptors to build its tree model.
 */
class FYUTILS_EXPORT AdvancedSettingsRegistry
{
public:
    //! Add or replace a descriptor with the same id.
    void add(AdvancedSettingDescriptor descriptor);

    //! Return a snapshot of all registered descriptors.
    [[nodiscard]] std::vector<AdvancedSettingDescriptor> descriptors() const;

private:
    std::vector<AdvancedSettingDescriptor> m_descriptors;
};
} // namespace Fooyin
