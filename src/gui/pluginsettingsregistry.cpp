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

#include "pluginsettingsregistry.h"

#include <QString>

#include <algorithm>
#include <ranges>

namespace Fooyin {
void PluginSettingsRegistry::registerProvider(const QString& pluginId, std::unique_ptr<PluginSettingsProvider> provider)
{
    if(!provider) {
        return;
    }

    const QString trimmedId = pluginId.trimmed();
    if(trimmedId.isEmpty()) {
        return;
    }

    if(const auto currentIt = m_providerById.find(trimmedId); currentIt != m_providerById.cend()) {
        auto it = std::ranges::find_if(m_providers, [currentProvider = currentIt->second](const auto& existing) {
            return existing.get() == currentProvider;
        });
        if(it != m_providers.end()) {
            currentIt->second = provider.get();
            *it               = std::move(provider);
            return;
        }
    }

    m_providerById.insert_or_assign(trimmedId, provider.get());
    m_providers.push_back(std::move(provider));
}

bool PluginSettingsRegistry::hasProvider(const QString& pluginId) const
{
    return m_providerById.contains(pluginId);
}

PluginSettingsProvider* PluginSettingsRegistry::providerFor(const QString& pluginId) const
{
    return m_providerById.contains(pluginId) ? m_providerById.at(pluginId) : nullptr;
}
} // namespace Fooyin
