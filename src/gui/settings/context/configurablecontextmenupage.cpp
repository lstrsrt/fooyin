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

#include "configurablecontextmenupage.h"

#include <algorithm>

#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTreeView>

using namespace Qt::StringLiterals;

namespace Fooyin {
ConfigurableContextMenuWidget::ConfigurableContextMenuWidget(QString description, DisabledIdsReader readDisabledIds,
                                                             DisabledIdsWriter writeDisabledIds,
                                                             ContextMenuSectionList sections, QWidget* parent)
    : SettingsPageWidget{}
    , m_description{std::move(description)}
    , m_readDisabledIds{std::move(readDisabledIds)}
    , m_writeDisabledIds{std::move(writeDisabledIds)}
{
    if(parent) {
        setParent(parent);
    }

    auto* layout = new QGridLayout(this);

    int descriptionRow{0};
    int descriptionColumnSpan{1};

    for(auto& section : sections) {
        auto* model = new ConfigurableContextMenuModel(this);
        auto* tree  = new QTreeView(this);

        tree->setModel(model);
        tree->setHeaderHidden(true);
        tree->setRootIsDecorated(true);
        tree->setUniformRowHeights(true);
        tree->header()->setStretchLastSection(true);

        int treeRow = section.row;
        if(!section.title.isEmpty()) {
            auto* label = new QLabel(section.title, this);
            layout->addWidget(label, section.row, section.column, 1, section.columnSpan);
            ++treeRow;
        }

        layout->addWidget(tree, treeRow, section.column, section.rowSpan, section.columnSpan);
        layout->setRowStretch(treeRow, 1);
        layout->setColumnStretch(section.column, 1);

        descriptionRow        = std::max(descriptionRow, treeRow + section.rowSpan);
        descriptionColumnSpan = std::max(descriptionColumnSpan, section.column + section.columnSpan);

        m_sections.push_back({.definition = std::move(section), .model = model, .tree = tree});
    }

    auto* descriptionLabel = new QLabel(u"🛈 "_s + m_description, this);
    descriptionLabel->setWordWrap(true);
    layout->addWidget(descriptionLabel, descriptionRow, 0, 1, descriptionColumnSpan);
}

QStringList ConfigurableContextMenuWidget::sectionNodeIds() const
{
    QStringList ids;

    for(const auto& section : m_sections) {
        ids.append(section.model->allNodeIds());
    }

    ids.removeDuplicates();
    return ids;
}

QStringList ConfigurableContextMenuWidget::sectionDisabledIds() const
{
    QStringList ids;

    for(const auto& section : m_sections) {
        ids.append(section.model->disabledIds());
    }

    ids.removeDuplicates();
    return ids;
}

void ConfigurableContextMenuWidget::load()
{
    const QStringList disabledIds = m_readDisabledIds ? m_readDisabledIds() : QStringList{};

    for(auto& section : m_sections) {
        section.model->rebuild(section.definition.nodeFactory ? section.definition.nodeFactory()
                                                              : ContextMenuNodeList{});
        section.model->applyDisabledIds(disabledIds);
    }
}

void ConfigurableContextMenuWidget::apply()
{
    if(m_writeDisabledIds) {
        m_writeDisabledIds(sectionNodeIds(), sectionDisabledIds());
    }
}

void ConfigurableContextMenuWidget::reset()
{
    if(m_writeDisabledIds) {
        m_writeDisabledIds(sectionNodeIds(), {});
    }
}
} // namespace Fooyin
