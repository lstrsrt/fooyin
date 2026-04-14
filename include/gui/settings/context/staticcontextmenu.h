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

#include <QCoreApplication>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <array>

namespace Fooyin {
struct FYGUI_EXPORT TranslatableText
{
    const char* context{nullptr};
    const char* sourceText{nullptr};
};

[[nodiscard]] inline QString translate(TranslatableText text)
{
    return (text.context && text.sourceText) ? QCoreApplication::translate(text.context, text.sourceText) : QString{};
}

namespace StaticContextMenu {
struct FYGUI_EXPORT Item
{
    const char* id;
    TranslatableText title;
    bool isSeparator;
};

template <size_t N>
QStringList defaultLayoutIds(const std::array<Item, N>& items)
{
    QStringList ids;
    ids.reserve(static_cast<qsizetype>(items.size()));

    for(const auto& item : items) {
        ids.emplace_back(QString::fromUtf8(item.id));
    }

    return ids;
}

template <size_t N>
bool isBuiltInSeparatorId(const std::array<Item, N>& items, const QString& id)
{
    return std::ranges::any_of(
        items, [&id](const auto& item) { return item.isSeparator && id == QString::fromUtf8(item.id); });
}
} // namespace StaticContextMenu
} // namespace Fooyin
