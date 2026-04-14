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

#include "configurablecontextmenupage.h"

#include "contextmenuids.h"

#include <span>

namespace Fooyin {
struct StaticContextMenuDescriptor
{
    const char* pageId;
    TranslatableText pageName;
    TranslatableText description;
    std::span<const ContextMenuIds::Item> items;
    ConfigurableContextMenuDefinition::StringListReader readDisabledIds;
    ConfigurableContextMenuDefinition::StringListWriter writeDisabledIds;
    ConfigurableContextMenuDefinition::StringListReader readTopLevelOrder;
    ConfigurableContextMenuDefinition::StringListWriter writeTopLevelOrder;
    bool allowReordering{true};
};

template <auto DisabledKey, auto LayoutKey, size_t N>
StaticContextMenuDescriptor
makeStaticContextMenuDescriptor(const char* pageId, TranslatableText pageName, TranslatableText description,
                                const std::array<ContextMenuIds::Item, N>& items, SettingsManager* settings)
{
    return {
        .pageId             = pageId,
        .pageName           = pageName,
        .description        = description,
        .items              = std::span<const ContextMenuIds::Item>{items},
        .readDisabledIds    = ContextMenuSettings::makeStringListReader<DisabledKey>(settings),
        .writeDisabledIds   = ContextMenuSettings::makeStringListWriter<DisabledKey>(settings),
        .readTopLevelOrder  = ContextMenuSettings::makeStringListReader<LayoutKey>(settings),
        .writeTopLevelOrder = ContextMenuSettings::makeStringListWriter<LayoutKey>(settings),
        .allowReordering    = true,
    };
}

class StaticContextMenuPage : public SettingsPage
{
    Q_OBJECT

public:
    StaticContextMenuPage(SettingsManager* settings, StaticContextMenuDescriptor descriptor, QObject* parent = nullptr);

private:
    static ContextMenuNodeList contextMenuNodes(std::span<const ContextMenuIds::Item> items);
};
} // namespace Fooyin
