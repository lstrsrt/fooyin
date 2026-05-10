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

#include <utils/settings/advancedsettingsregistry.h>

#include <algorithm>

namespace Fooyin {
void AdvancedSettingsRegistry::add(AdvancedSettingDescriptor descriptor)
{
    if(descriptor.id.isEmpty()) {
        return;
    }

    const auto existing = std::ranges::find(m_descriptors, descriptor.id, &AdvancedSettingDescriptor::id);
    if(existing != m_descriptors.end()) {
        *existing = std::move(descriptor);
        return;
    }

    m_descriptors.emplace_back(std::move(descriptor));
}

std::vector<AdvancedSettingDescriptor> AdvancedSettingsRegistry::descriptors() const
{
    return m_descriptors;
}
} // namespace Fooyin
