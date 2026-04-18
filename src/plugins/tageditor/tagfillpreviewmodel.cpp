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

#include "tagfillpreviewmodel.h"

#include <QApplication>
#include <QBrush>
#include <QPalette>

using namespace Qt::StringLiterals;

namespace Fooyin::TagEditor {
FillPreviewModel::FillPreviewModel(QObject* parent)
    : QAbstractTableModel{parent}
{ }

void FillPreviewModel::setPreviewData(FillValuesResult result, const QString& sourceColumnTitle,
                                      const QStringList& fieldTitles)
{
    beginResetModel();

    m_result  = std::move(result);
    m_headers = {u"#"_s, sourceColumnTitle};
    m_headers.append(fieldTitles);

    endResetModel();
}

QVariant FillPreviewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation != Qt::Horizontal || section < 0 || section >= m_headers.size()) {
        return {};
    }

    if(role == Qt::DisplayRole) {
        return m_headers.at(section);
    }

    if(role == Qt::TextAlignmentRole) {
        return Qt::AlignCenter;
    }

    return {};
}

QVariant FillPreviewModel::data(const QModelIndex& index, int role) const
{
    if(!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }

    const int row = index.row();
    if(row < 0 || std::cmp_greater_equal(row, m_result.previewRows.size())) {
        return {};
    }

    const auto& previewRow = m_result.previewRows.at(static_cast<size_t>(row));

    if(role == Qt::DisplayRole) {
        if(index.column() == 0) {
            return row + 1;
        }
        if(index.column() == 1) {
            return previewRow.source;
        }

        const int valueIndex = index.column() - 2;
        if(valueIndex >= 0 && std::cmp_less(valueIndex, previewRow.values.size())) {
            return previewRow.values.at(static_cast<size_t>(valueIndex));
        }
        return {};
    }

    if(role == Qt::TextAlignmentRole && index.column() == 0) {
        return Qt::AlignCenter;
    }

    if(role == Qt::ForegroundRole && !previewRow.matched && m_result.patternValid) {
        return QBrush{QApplication::palette().color(QPalette::Disabled, QPalette::Text)};
    }

    return {};
}

int FillPreviewModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid()) {
        return 0;
    }

    return static_cast<int>(m_result.previewRows.size());
}

int FillPreviewModel::columnCount(const QModelIndex& parent) const
{
    if(parent.isValid()) {
        return 0;
    }

    return static_cast<int>(m_headers.size());
}
} // namespace Fooyin::TagEditor
