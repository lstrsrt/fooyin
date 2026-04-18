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

#include "tagfilloperation.h"

#include <QAbstractTableModel>
#include <QStringList>

namespace Fooyin::TagEditor {
class FillPreviewModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit FillPreviewModel(QObject* parent = nullptr);

    void setPreviewData(FillValuesResult result, const QString& sourceColumnTitle, const QStringList& fieldTitles);

    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent) const override;

private:
    QStringList m_headers;
    FillValuesResult m_result;
};
} // namespace Fooyin::TagEditor
