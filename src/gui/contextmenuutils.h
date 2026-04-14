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

#include <QMenu>

namespace Fooyin::ContextMenuUtils {
template <typename Range, typename IsBoundary, typename RenderItem>
void renderGroupedMenu(QMenu* menu, const Range& items, IsBoundary&& isBoundary, RenderItem&& renderItem)
{
    if(!menu) {
        return;
    }

    bool hasRenderedContent{false};
    bool pendingBoundary{false};

    for(const auto& item : items) {
        if(std::forward<IsBoundary>(isBoundary)(item)) {
            if(hasRenderedContent) {
                pendingBoundary = true;
            }
            continue;
        }

        if(pendingBoundary) {
            QAction* separator = menu->addSeparator();
            if(!std::forward<RenderItem>(renderItem)(item, menu)) {
                menu->removeAction(separator);
                separator->deleteLater();
                continue;
            }

            pendingBoundary    = false;
            hasRenderedContent = true;
            continue;
        }

        if(std::forward<RenderItem>(renderItem)(item, menu)) {
            hasRenderedContent = true;
        }
    }
}
} // namespace Fooyin::ContextMenuUtils
