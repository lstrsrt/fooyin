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

#include "configurablecontextmenumodel.h"

#include <ranges>
#include <utility>

using namespace Qt::StringLiterals;

namespace {
QString displayTitle(const QString& title)
{
    QString result;
    result.reserve(title.size());

    for(qsizetype i{0}; i < title.size(); ++i) {
        const auto ch = title.at(i);
        if(ch != '&'_L1) {
            result.append(ch);
            continue;
        }

        if(i + 1 < title.size() && title.at(i + 1) == '&'_L1) {
            result.append('&'_L1);
            ++i;
        }
    }

    return result;
}
} // namespace

namespace Fooyin {
ConfigurableContextMenuItem::ConfigurableContextMenuItem()
    : ConfigurableContextMenuItem{{}, {}, nullptr}
{ }

ConfigurableContextMenuItem::ConfigurableContextMenuItem(QString id, const QString& title,
                                                         ConfigurableContextMenuItem* parent)
    : TreeItem{parent}
    , m_id{std::move(id)}
    , m_title{displayTitle(title)}
    , m_checked{Qt::Checked}
{ }

QString ConfigurableContextMenuItem::id() const
{
    return m_id;
}

QString ConfigurableContextMenuItem::title() const
{
    return m_title;
}

Qt::CheckState ConfigurableContextMenuItem::checked() const
{
    return m_checked;
}

void ConfigurableContextMenuItem::setChecked(Qt::CheckState checked)
{
    m_checked = checked;
}

ConfigurableContextMenuModel::ConfigurableContextMenuModel(QObject* parent)
    : TreeModel{parent}
    , m_updating{false}
{ }

void ConfigurableContextMenuModel::rebuild(const ContextMenuNodeList& nodes)
{
    beginResetModel();

    resetRoot();
    m_items.clear();

    for(const auto& node : nodes) {
        auto* parent = rootItem();
        if(!node.parentId.isEmpty() && m_items.contains(node.parentId)) {
            parent = m_items.at(node.parentId).get();
        }

        auto item = std::make_unique<ConfigurableContextMenuItem>(node.id, node.title, parent);
        parent->appendChild(item.get());
        m_items.emplace(item->id(), std::move(item));
    }

    endResetModel();
}

void ConfigurableContextMenuModel::applyDisabledIds(const QStringList& disabledIds)
{
    beginResetModel();

    setChildState(rootItem(), Qt::Checked);

    for(const auto& id : disabledIds) {
        if(!m_items.contains(id)) {
            continue;
        }

        auto* item = m_items.at(id).get();
        item->setChecked(Qt::Unchecked);
        setChildState(item, Qt::Unchecked);
    }

    updateParentStates(rootItem());

    endResetModel();
}

QStringList ConfigurableContextMenuModel::disabledIds() const
{
    QStringList ids;

    for(const auto& item : m_items | std::views::values) {
        if(item->checked() == Qt::Unchecked) {
            ids.append(item->id());
        }
    }

    return ids;
}

QStringList ConfigurableContextMenuModel::allNodeIds() const
{
    QStringList ids;

    for(const auto& item : m_items | std::views::values) {
        ids.append(item->id());
    }

    return ids;
}

Qt::ItemFlags ConfigurableContextMenuModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags itemFlags = QAbstractItemModel::flags(index);

    if(index.isValid()) {
        itemFlags |= Qt::ItemIsUserCheckable;
    }

    return itemFlags;
}

int ConfigurableContextMenuModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 1;
}

QVariant ConfigurableContextMenuModel::data(const QModelIndex& index, int role) const
{
    if(!checkIndex(index, CheckIndexOption::IndexIsValid)) {
        return {};
    }

    const auto* item = itemForIndex(index);

    if(role == Qt::DisplayRole) {
        return item->title();
    }
    if(role == Qt::CheckStateRole) {
        return item->checked();
    }

    return {};
}

bool ConfigurableContextMenuModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(role != Qt::CheckStateRole || !index.isValid() || m_updating) {
        return false;
    }

    auto* item       = itemForIndex(index);
    const auto state = static_cast<Qt::CheckState>(value.toInt());

    if(state == Qt::PartiallyChecked || item->checked() == state) {
        return false;
    }

    m_updating = true;

    item->setChecked(state);
    setChildState(item, state);
    updateParentState(item->parent());
    emitDataChangedRecursive(item);

    for(auto* parentItem = item->parent(); parentItem && parentItem != rootItem(); parentItem = parentItem->parent()) {
        const QModelIndex idx = indexOfItem(parentItem);
        emit dataChanged(idx, idx, {Qt::CheckStateRole});
    }

    m_updating = false;
    return true;
}

void ConfigurableContextMenuModel::setChildState(ConfigurableContextMenuItem* parent, Qt::CheckState state)
{
    for(const auto& child : parent->children()) {
        child->setChecked(state);
        setChildState(child, state);
    }
}

void ConfigurableContextMenuModel::updateParentState(ConfigurableContextMenuItem* item)
{
    if(!item || item == rootItem() || item->childCount() == 0) {
        return;
    }

    int checked{0};
    int unchecked{0};
    int partial{0};

    for(const auto& child : item->children()) {
        switch(child->checked()) {
            case Qt::Checked:
                ++checked;
                break;
            case Qt::Unchecked:
                ++unchecked;
                break;
            case Qt::PartiallyChecked:
                ++partial;
                break;
        }
    }

    if(partial > 0 || (checked > 0 && unchecked > 0)) {
        item->setChecked(Qt::PartiallyChecked);
    }
    else if(unchecked > 0 && checked == 0) {
        item->setChecked(Qt::Unchecked);
    }
    else {
        item->setChecked(Qt::Checked);
    }

    updateParentState(item->parent());
}

void ConfigurableContextMenuModel::updateParentStates(ConfigurableContextMenuItem* item)
{
    for(const auto& child : item->children()) {
        updateParentStates(child);
    }

    if(item != rootItem()) {
        updateParentState(item);
    }
}

void ConfigurableContextMenuModel::emitDataChangedRecursive(ConfigurableContextMenuItem* item)
{
    if(item == rootItem()) {
        return;
    }

    const QModelIndex idx = indexOfItem(item);
    emit dataChanged(idx, idx, {Qt::CheckStateRole});

    for(const auto& child : item->children()) {
        emitDataChangedRecursive(child);
    }
}
} // namespace Fooyin
