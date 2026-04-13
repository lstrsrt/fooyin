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

#include "configurablecontextmenumodel.h"

#include <utils/settings/settingspage.h>

#include <functional>

class QTreeView;

namespace Fooyin {
struct ConfigurableContextMenuSection
{
    QString title;
    std::function<std::vector<ConfigurableContextMenuNode>()> nodeFactory;
    int row{0};
    int column{0};
    int rowSpan{1};
    int columnSpan{1};
};
using ContextMenuSectionList = std::vector<ConfigurableContextMenuSection>;

class ConfigurableContextMenuWidget : public SettingsPageWidget
{
    Q_OBJECT

public:
    using DisabledIdsReader = std::function<QStringList()>;
    using DisabledIdsWriter = std::function<void(const QStringList&, const QStringList&)>;

    ConfigurableContextMenuWidget(QString description, DisabledIdsReader readDisabledIds,
                                  DisabledIdsWriter writeDisabledIds, ContextMenuSectionList sections,
                                  QWidget* parent = nullptr);

    void load() override;
    void apply() override;
    void reset() override;

private:
    struct SectionView
    {
        ConfigurableContextMenuSection definition;
        ConfigurableContextMenuModel* model;
        QTreeView* tree;
    };

    [[nodiscard]] QStringList sectionNodeIds() const;
    [[nodiscard]] QStringList sectionDisabledIds() const;

    QString m_description;
    DisabledIdsReader m_readDisabledIds;
    DisabledIdsWriter m_writeDisabledIds;
    QList<SectionView> m_sections;
};
} // namespace Fooyin
