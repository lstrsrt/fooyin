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

#include "fygui_export.h"

#include <QString>
#include <QWidget>

class QLabel;
class QLineEdit;

namespace Fooyin {
class FYGUI_EXPORT LineEditEditor : public QWidget
{
    Q_OBJECT

public:
    explicit LineEditEditor(const QString& name, QWidget* parent = nullptr);
    explicit LineEditEditor(QWidget* parent = nullptr);

    [[nodiscard]] QString text() const;
    void setText(const QString& text);
    void clear();

    [[nodiscard]] QString placeholderText() const;
    void setPlaceholderText(const QString& text);

    [[nodiscard]] QLabel* label() const;
    [[nodiscard]] QLineEdit* lineEdit() const;

Q_SIGNALS:
    void textChanged(const QString& text);
    void textEdited(const QString& text);
    void returnPressed();
    void editingFinished();

private:
    QLabel* m_label;
    QLineEdit* m_lineEdit;
};
} // namespace Fooyin
