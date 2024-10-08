/*
 * Fooyin
 * Copyright © 2023, Luke Taylor <LukeT1@proton.me>
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

#include "settingscategory.h"

#include <utils/id.h>
#include <utils/treeitem.h>
#include <utils/treemodel.h>

#include <QAbstractListModel>
#include <QIcon>

#include <set>

namespace Fooyin {
class SettingsItem : public TreeItem<SettingsItem>
{
public:
    enum Role
    {
        Data = Qt::UserRole,
    };

    SettingsItem();
    SettingsItem(SettingsCategory* data, SettingsItem* parent);

    bool operator<(const SettingsItem& other) const;

    SettingsCategory* data() const;

    void sort();

private:
    SettingsCategory* m_data;
};

class SettingsModel : public TreeModel<SettingsItem>
{
public:
    explicit SettingsModel(QObject* parent = nullptr);

    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;

    void setPages(const PageList& pages);

    SettingsCategory* categoryForPage(const Id& page);
    [[nodiscard]] QModelIndex indexForCategory(const Id& categoryId) const;

private:
    using CategoryIdMap = std::map<Id, SettingsCategory>;
    using ItemIdMap     = std::map<Id, SettingsItem>;

    CategoryIdMap m_categories;
    ItemIdMap m_items;
    std::set<Id> m_pageIds;
};
} // namespace Fooyin
