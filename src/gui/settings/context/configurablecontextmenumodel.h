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

#include <utils/treeitem.h>
#include <utils/treemodel.h>

#include <QMimeData>
#include <QString>
#include <QStringList>

#include <unordered_map>

namespace Fooyin {
struct ConfigurableContextMenuNode
{
    QString id;
    QString title;
    QString parentId;
    bool isSeparator{false};
};
using ContextMenuNodeList = std::vector<ConfigurableContextMenuNode>;

class ConfigurableContextMenuItem : public TreeItem<ConfigurableContextMenuItem>
{
public:
    ConfigurableContextMenuItem();
    ConfigurableContextMenuItem(QString id, const QString& title, bool isSeparator,
                                ConfigurableContextMenuItem* parent);

    [[nodiscard]] QString id() const;
    [[nodiscard]] QString title() const;
    [[nodiscard]] Qt::CheckState checked() const;
    [[nodiscard]] bool isSeparator() const;

    void setChecked(Qt::CheckState checked);

private:
    QString m_id;
    QString m_title;
    Qt::CheckState m_checked;
    bool m_isSeparator;
};

class ConfigurableContextMenuModel : public TreeModel<ConfigurableContextMenuItem>
{
    Q_OBJECT

public:
    explicit ConfigurableContextMenuModel(QObject* parent = nullptr);

    void setReorderingEnabled(bool enabled);

    void rebuild(const ContextMenuNodeList& nodes, const QStringList& topLevelLayout = {});
    void applyDisabledIds(const QStringList& disabledIds);
    [[nodiscard]] QStringList disabledIds() const;
    [[nodiscard]] QStringList allNodeIds() const;
    [[nodiscard]] QStringList topLevelLayoutIds() const;
    [[nodiscard]] bool isSeparator(const QModelIndex& index) const;
    [[nodiscard]] bool canInsertSeparator(int row) const;

    void insertSeparator(int row);
    void removeSeparator(const QModelIndex& index);

    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
                         const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
                      const QModelIndex& parent) override;
    [[nodiscard]] Qt::DropActions supportedDropActions() const override;
    [[nodiscard]] Qt::DropActions supportedDragActions() const override;

private:
    void setChildState(ConfigurableContextMenuItem* parent, Qt::CheckState state);
    void updateParentState(ConfigurableContextMenuItem* item);
    void updateParentStates(ConfigurableContextMenuItem* item);
    void emitDataChangedRecursive(ConfigurableContextMenuItem* item);
    [[nodiscard]] ConfigurableContextMenuItem* itemForId(const QString& id) const;

    std::unordered_map<QString, std::unique_ptr<ConfigurableContextMenuItem>> m_items;
    bool m_updating;
    bool m_reorderingEnabled;
};
} // namespace Fooyin
