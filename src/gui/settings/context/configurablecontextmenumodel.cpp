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

#include <gui/contextmenuutils.h>

#include <QDataStream>
#include <QIODevice>
#include <QUuid>

#include <functional>
#include <ranges>
#include <utility>

using namespace Qt::StringLiterals;

constexpr auto ContextMenuItemsMimeType = "application/x-fooyin-contextmenu-items";
constexpr auto SeparatorIdPrefix        = "separator:";

namespace Fooyin {
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

bool isSeparatorId(const QString& id)
{
    return id.startsWith(QLatin1StringView{SeparatorIdPrefix});
}

QString separatorId()
{
    return QString::fromLatin1("%1%2")
        .arg(QLatin1StringView{SeparatorIdPrefix})
        .arg(QUuid::createUuid().toString(QUuid::Id128));
}

QString separatorTitle()
{
    return QObject::tr("Separator");
}

ContextMenuNodeList layoutNodes(const ContextMenuNodeList& nodes, const QStringList& topLevelLayout)
{
    std::unordered_map<QString, ContextMenuNodeList> childrenByParent;
    ContextMenuNodeList topLevelNodes;

    for(const auto& node : nodes) {
        if(node.parentId.isEmpty()) {
            topLevelNodes.emplace_back(node);
        }
        else {
            childrenByParent[node.parentId].emplace_back(node);
        }
    }

    QStringList defaultLayout;
    defaultLayout.reserve(static_cast<qsizetype>(topLevelNodes.size()));

    for(const auto& node : topLevelNodes) {
        defaultLayout.emplace_back(node.id);
    }

    const QStringList effectiveLayout = topLevelLayout.isEmpty()
                                          ? defaultLayout
                                          : ContextMenuUtils::effectiveContextMenuLayout(defaultLayout, topLevelLayout);

    ContextMenuNodeList orderedNodes;
    orderedNodes.reserve(nodes.size() + static_cast<size_t>(std::ranges::count_if(effectiveLayout, [](const auto& id) {
                             return isSeparatorId(id);
                         })));

    std::function<void(const ConfigurableContextMenuNode&)> appendNode = [&](const ConfigurableContextMenuNode& node) {
        orderedNodes.push_back(node);

        const auto childIt = childrenByParent.find(node.id);
        if(childIt == childrenByParent.cend()) {
            return;
        }

        for(const auto& child : childIt->second) {
            appendNode(child);
        }
    };

    for(const auto& entry : std::as_const(effectiveLayout)) {
        if(isSeparatorId(entry)) {
            orderedNodes.push_back({.id = entry, .title = separatorTitle(), .parentId = {}, .isSeparator = true});
            continue;
        }

        const auto rootIt
            = std::ranges::find_if(topLevelNodes, [&entry](const auto& node) { return node.id == entry; });
        if(rootIt == topLevelNodes.cend()) {
            continue;
        }

        appendNode(*rootIt);
    }

    return orderedNodes;
}
} // namespace

ConfigurableContextMenuItem::ConfigurableContextMenuItem()
    : ConfigurableContextMenuItem{{}, {}, false, nullptr}
{ }

ConfigurableContextMenuItem::ConfigurableContextMenuItem(QString id, const QString& title, bool isSeparator,
                                                         ConfigurableContextMenuItem* parent)
    : TreeItem{parent}
    , m_id{std::move(id)}
    , m_title{isSeparator ? separatorTitle() : displayTitle(title)}
    , m_checked{Qt::Checked}
    , m_isSeparator{isSeparator}
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

bool ConfigurableContextMenuItem::isSeparator() const
{
    return m_isSeparator;
}

void ConfigurableContextMenuItem::setChecked(Qt::CheckState checked)
{
    m_checked = checked;
}

ConfigurableContextMenuModel::ConfigurableContextMenuModel(QObject* parent)
    : TreeModel{parent}
    , m_updating{false}
    , m_reorderingEnabled{false}
{ }

void ConfigurableContextMenuModel::setReorderingEnabled(bool enabled)
{
    m_reorderingEnabled = enabled;
}

void ConfigurableContextMenuModel::rebuild(const ContextMenuNodeList& nodes, const QStringList& topLevelLayout)
{
    beginResetModel();

    resetRoot();
    m_items.clear();

    for(const auto& node : layoutNodes(nodes, topLevelLayout)) {
        auto* parent = rootItem();
        if(!node.parentId.isEmpty() && m_items.contains(node.parentId)) {
            parent = m_items.at(node.parentId).get();
        }

        auto item = std::make_unique<ConfigurableContextMenuItem>(node.id, node.title, node.isSeparator, parent);
        parent->appendChild(item.get());
        m_items.emplace(item->id(), std::move(item));
    }

    rootItem()->resetChildren();
    endResetModel();
}

void ConfigurableContextMenuModel::applyDisabledIds(const QStringList& disabledIds)
{
    beginResetModel();

    setChildState(rootItem(), Qt::Checked);

    for(const auto& id : disabledIds) {
        auto* item = itemForId(id);
        if(!item || item->isSeparator()) {
            continue;
        }

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
        if(item->isSeparator()) {
            continue;
        }

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
        if(item->isSeparator()) {
            continue;
        }
        ids.append(item->id());
    }

    return ids;
}

QStringList ConfigurableContextMenuModel::topLevelLayoutIds() const
{
    QStringList ids;

    for(const auto* item : rootItem()->children()) {
        ids.append(item->id());
    }

    return ids;
}

bool ConfigurableContextMenuModel::isSeparator(const QModelIndex& index) const
{
    return index.isValid() && itemForIndex(index)->isSeparator();
}

bool ConfigurableContextMenuModel::canInsertSeparator(int row) const
{
    const int insertRow = std::clamp(row, 0, rootItem()->childCount());

    if(insertRow > 0) {
        const auto* previousItem = rootItem()->child(insertRow - 1);
        if(previousItem && previousItem->isSeparator()) {
            return false;
        }
    }

    if(insertRow < rootItem()->childCount()) {
        const auto* nextItem = rootItem()->child(insertRow);
        if(nextItem && nextItem->isSeparator()) {
            return false;
        }
    }

    return true;
}

void ConfigurableContextMenuModel::insertSeparator(int row)
{
    const int insertRow = std::clamp(row, 0, rootItem()->childCount());
    if(!canInsertSeparator(insertRow)) {
        return;
    }

    const QString id    = separatorId();

    beginInsertRows({}, insertRow, insertRow);

    auto item = std::make_unique<ConfigurableContextMenuItem>(id, separatorTitle(), true, rootItem());
    rootItem()->insertChild(insertRow, item.get());
    m_items.emplace(id, std::move(item));
    rootItem()->resetChildren();

    endInsertRows();
}

void ConfigurableContextMenuModel::removeSeparator(const QModelIndex& index)
{
    if(!index.isValid()) {
        return;
    }

    auto* item = itemForIndex(index);
    if(item->parent() != rootItem() || !item->isSeparator()) {
        return;
    }

    const QString id = item->id();

    beginRemoveRows({}, index.row(), index.row());
    rootItem()->removeChild(index.row());
    rootItem()->resetChildren();
    endRemoveRows();

    m_items.erase(id);
}

Qt::ItemFlags ConfigurableContextMenuModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags itemFlags = QAbstractItemModel::flags(index);

    if(index.isValid()) {
        const auto* item = itemForIndex(index);
        if(!item->isSeparator()) {
            itemFlags |= Qt::ItemIsUserCheckable;
        }

        if(m_reorderingEnabled && item->parent() == rootItem()) {
            itemFlags |= Qt::ItemIsDragEnabled;
        }
    }
    else if(m_reorderingEnabled) {
        itemFlags |= Qt::ItemIsDropEnabled;
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
    if(role == Qt::CheckStateRole && !item->isSeparator()) {
        return item->checked();
    }

    return {};
}

bool ConfigurableContextMenuModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if(role != Qt::CheckStateRole || !index.isValid() || m_updating) {
        return false;
    }

    auto* item = itemForIndex(index);
    if(item->isSeparator()) {
        return false;
    }

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

QStringList ConfigurableContextMenuModel::mimeTypes() const
{
    return {QString::fromLatin1(ContextMenuItemsMimeType)};
}

QMimeData* ConfigurableContextMenuModel::mimeData(const QModelIndexList& indexes) const
{
    QStringList ids;

    for(const auto& index : indexes) {
        if(!index.isValid()) {
            continue;
        }

        const auto* item = itemForIndex(index);
        if(item->parent() == rootItem() && !ids.contains(item->id())) {
            ids.emplace_back(item->id());
        }
    }

    auto* mimeData = new QMimeData();

    QByteArray data;
    QDataStream stream{&data, QIODevice::WriteOnly};

    stream << ids;

    mimeData->setData(QString::fromLatin1(ContextMenuItemsMimeType), data);
    return mimeData;
}

bool ConfigurableContextMenuModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
                                                   const QModelIndex& parent) const
{
    if(!m_reorderingEnabled || action != Qt::MoveAction || parent.isValid() || column > 0) {
        return false;
    }

    return data->hasFormat(QString::fromLatin1(ContextMenuItemsMimeType))
        && (row < 0 || row <= rootItem()->childCount());
}

bool ConfigurableContextMenuModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
                                                const QModelIndex& parent)
{
    if(!canDropMimeData(data, action, row, column, parent)) {
        return false;
    }

    QByteArray encodedData = data->data(QString::fromLatin1(ContextMenuItemsMimeType));
    QDataStream stream{&encodedData, QIODevice::ReadOnly};

    QStringList movedIds;
    stream >> movedIds;

    if(movedIds.isEmpty()) {
        return false;
    }

    if(row < 0) {
        row = rootItem()->childCount();
    }

    beginResetModel();

    auto* root = rootItem();
    for(const auto& id : movedIds) {
        const auto children = root->children();
        const auto childIt
            = std::ranges::find_if(children, [&id](const auto* item) { return item && item->id() == id; });
        if(childIt == children.cend()) {
            continue;
        }

        const int currentIndex = static_cast<int>(std::ranges::distance(children.cbegin(), childIt));
        root->moveChild(currentIndex, row);
        ++row;
    }

    root->resetChildren();
    endResetModel();
    return true;
}

Qt::DropActions ConfigurableContextMenuModel::supportedDropActions() const
{
    return m_reorderingEnabled ? Qt::MoveAction : Qt::IgnoreAction;
}

Qt::DropActions ConfigurableContextMenuModel::supportedDragActions() const
{
    return m_reorderingEnabled ? Qt::MoveAction : Qt::IgnoreAction;
}

void ConfigurableContextMenuModel::setChildState(ConfigurableContextMenuItem* parent, Qt::CheckState state)
{
    for(const auto& child : parent->children()) {
        if(child->isSeparator()) {
            continue;
        }

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
        if(child->isSeparator()) {
            continue;
        }

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
        if(child->isSeparator()) {
            continue;
        }
        updateParentStates(child);
    }

    if(item != rootItem() && !item->isSeparator()) {
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
        if(child->isSeparator()) {
            continue;
        }
        emitDataChangedRecursive(child);
    }
}

ConfigurableContextMenuItem* ConfigurableContextMenuModel::itemForId(const QString& id) const
{
    const auto it = m_items.find(id);
    return it != m_items.cend() ? it->second.get() : nullptr;
}
} // namespace Fooyin
